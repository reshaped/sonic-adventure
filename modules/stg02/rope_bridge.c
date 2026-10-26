/* O TURI1, O TURIBR, O TURIBR2 and O E102BR: rope bridges that bridge_init
 * builds. O TURI1 stands, and O TURIBR and O TURIBR2 drop their planks.
 * O E102BR has a sai, the rhino enemy, under it, and breaks once a hit
 * damages the sai. O STBRID and O YURE: models */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "effect/explosion.h"
#include "enemy/enemy.h"
#include "enemy/sai.h"
#include "enemy/sai_textures.h"
#include "object/bridge.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/lockon.h"
#include "sound/sound.h"
#include "stg02/bridge.h"
#include "stg02/rope_bridge.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_TEXLIST obj_windy_texlist;
extern NJS_MODEL windy_rope_bridge_model;
extern NJS_MODEL windy_rope_bridge_2_model;
extern NJS_MODEL windy_rope_bridge_post_model;
extern NJS_MODEL windy_rope_bridge_rope_model;
extern NJS_MODEL windy_rope_bridge_plank_model;
extern NJS_OBJECT sai_object;
extern NJS_OBJECT windy_stbrid_object;
extern NJS_ACTION windy_yure_action;

/* a falling bridge's work->submode. past FALLING_BROKEN, it counts on as the
 * bridge sinks */
enum falling_mode { FALLING_WHOLE, FALLING_COLLAPSE, FALLING_BROKEN };
/* a sai bridge's work->submode, which counts on past SAI_BROKEN the same way */
enum sai_mode { SAI_WAIT, SAI_HIT, SAI_BROKEN };

/* O YURE's modes. in YURE_STOP and YURE_END its step does nothing, and no step
 * here sets either */
enum yure_mode { YURE_START, YURE_RUN, YURE_STOP, YURE_END };

struct yure_vars {
    Float frame;
};

#define YURE_VARS(work) ((struct yure_vars *)&(work)->vars)

/* the bridges' collision reach, far past their shapes */
#define BRIDGE_REACH 250000.0F

/* the plank the collapse spreads out from. a plank d planks from it gets a
 * tick of COLLAPSE_WAIT plus d * d * COLLAPSE_SPREAD, rounded down */
#define COLLAPSE_MIDDLE 40
#define COLLAPSE_WAIT 2
#define COLLAPSE_SPREAD 0.025F

/* as a falling bridge sinks, it leaves the collision world's tests once
 * work->submode passes FALLING_SOLID. any sinking bridge destroys itself
 * once submode passes SINK_STEPS */
#define FALLING_SOLID 30
#define SINK_STEPS 100

/* the second break of a sai bridge lies this far along x from the first */
#define BREAK_GAP 70.0F

#define SE_COLLAPSE 64

static void falling_bridge_step(struct task *task);
static void falling_bridge_collapse(struct task_work *work);
static void sai_bridge_step(struct task *task);
static void sai_bridge_end(struct task *task);
static void windy_yure_draw(struct task *task);
static void windy_yure_destroy(struct task *task);

/* bridge_init's defaults, nine floats of struct bridge_params' ten. in the
 * image, windy_rope_bridge_parts follows with no room for a tenth */
static Float windy_rope_bridge_params[9] = {
    170.0F, 2.5F, -0.12F, 16.0F, 11.5F, 0.04F, 0.0065F, 17.5F, -1.3F,
};

#define PARAMS ((struct bridge_params *)windy_rope_bridge_params)

static struct bridge_parts windy_rope_bridge_parts = {
    {{0, 1, 2, 3}, {4, 5, 6, 7}},
    {{136, 137, 138, 139}, {140, 141, 142, 143}},
    {0, 1, 2, 3},
    {{0.0F, 101.81F}, {-30.0F, 72.0F}, {-17.7F, 17.7F}},
    101.81F,
    1.0F,
    &obj_windy_texlist,
    &windy_rope_bridge_model,
    &windy_rope_bridge_post_model,
    &windy_rope_bridge_rope_model,
    &windy_rope_bridge_plank_model,
    PARAMS,
};
static struct bridge_parts windy_rope_bridge_2_parts = {
    {{0, 1, 2, 3}, {4, 5, 6, 7}},
    {{204, 205, 206, 207}, {208, 209, 210, 211}},
    {0, 1, 2, 3},
    {{0.0F, 150.499F}, {-30.0F, 72.0F}, {-17.7F, 17.7F}},
    150.499F,
    1.0F,
    &obj_windy_texlist,
    &windy_rope_bridge_2_model,
    &windy_rope_bridge_post_model,
    &windy_rope_bridge_rope_model,
    &windy_rope_bridge_plank_model,
    PARAMS,
};

/* each falling bridge's trigger, a sphere 100 along the bridge and 20 below
 * it. O TURIBR2's longer bridge adds a box past its far end. the box pushes
 * back players, enemies, objects and pickups */
static struct collision_shape falling_bridge_shapes[1] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS,
     PUSH_FIRM(PUSH_ALL),
     {100.0F, -20.0F, 0.0F},
     20.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};
static struct collision_shape falling_bridge_2_shapes[2] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS,
     PUSH_FIRM(PUSH_ALL),
     {100.0F, -20.0F, 0.0F},
     20.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {230.0F, -50.0F, 0.0F},
     60.0F,
     5.0F,
     15.0F,
     {0},
     0,
     NULL},
};
/* the sai's sphere, centered where it bursts */
static struct collision_shape sai_bridge_shapes[1] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_HURTS_PLAYERS,
     PUSH_FIRM(PUSH_ALL),
     {100.0F, -20.0F, 0.0F},
     5.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

static const NJS_POINT3 plank_origin = {0.0F, 0.0F, 0.0F};
static const NJS_VECTOR collapse_dust_speed = {-2.0F, 0.0F, -0.5F};
/* in the bridge's frame: where the sai stands, and where it bursts */
static const NJS_POINT3 sai_place = {100.0F, -30.0F, 0.0F};
static const NJS_POINT3 sai_burst = {100.0F, -20.0F, 0.0F};
/* where the bridge breaks first, in its frame. the loop sets x for each
 * break, so only y and z count */
static const NJS_POINT3 break_place = {0.0F, 0.0F, 0.0F};
/* a word of 0 the image holds here, which no code reads. its type is
 * unknown */
/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const Uint32 dat_0ca15744 = 0;

void windy_rope_bridge_step(struct task *task)
{
    BRIDGE_VARS(task->work)->parts = &windy_rope_bridge_parts;
    bridge_init(task);
}

void windy_falling_bridge_step(struct task *task)
{
    BRIDGE_VARS(task->work)->parts = &windy_rope_bridge_parts;
    collision_init(task, falling_bridge_shapes, COLLISION_OBJECT, 1, 1);
    task->work->collision->radius = BRIDGE_REACH;
    bridge_init(task);
    task->step = falling_bridge_step;
}

void windy_falling_bridge_2_step(struct task *task)
{
    BRIDGE_VARS(task->work)->parts = &windy_rope_bridge_2_parts;
    collision_init(task, falling_bridge_2_shapes, COLLISION_OBJECT, 1, 2);
    task->work->collision->radius = BRIDGE_REACH;
    bridge_init(task);
    task->step = falling_bridge_step;
}

/* whole until a player, an attack or an enemy touches the trigger. then the
 * planks fall, and the bridge sinks, turning, until it destroys itself */
static void falling_bridge_step(struct task *task)
{
    struct task_work *work = task->work;

    if (edit_mode) {
        bridge_edit_draw(work);
    } else {
        switch (work->submode) {
        case FALLING_WHOLE:
            bridge_step_whole(task);
            if (work->collision->flags & COLLISION_HIT) {
                /* @bug logic: the store sets every flag but
                 * COLLISION_HIT, where a clear of COLLISION_HIT alone belongs
                 * (H'FFFD stored by MOV.W R0,@(2,R2) at 0x0c90962a).
                 * collision_register clears the hit flags, and the rest stay
                 * set. COLLISION_TARGET is among them, so the checks list the
                 * bridge's record as a target */
                work->collision->flags = (Uint16)~COLLISION_HIT;
                if (work->collision->own_shape == 0) {
                    work->submode++;
                }
            }
            collision_register(work);
            break;
        case FALLING_COLLAPSE:
            falling_bridge_collapse(work);
            bridge_step_broken(task);
            work->submode++;
            sound_play(SE_COLLAPSE, 0, 0, 0);
            break;
        case FALLING_BROKEN:
            bridge_step_broken(task);
            work->submode++;
            if (work->ang.z++ > 200) {
                work->ang.z = 0;
            }
            break;
        default:
            bridge_step_broken(task);
            work->pos.y -= 2.0F;
            work->ang.y += 15;
            if (work->submode++ > FALLING_SOLID) {
                work->flags &= ~WORLD_TESTED;
            }
            if (work->ang.z++ > 200) {
                work->ang.z = 0;
            }
            if (work->submode > SINK_STEPS) {
                placement_kill(task);
            }
            break;
        }
    }
    /* @hack redundant: a return where control returns anyway, which keeps the
     * store of ang.z ahead of the BRA at 0x0c9096da and a NOP in its slot;
     * without it, SHC moves the store into the slot, the function runs 2
     * bytes short, and 53 of 132 words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

/* bridge_collapse's drop, with two changes. each plank's x and z speeds are
 * -0.2, where bridge_collapse's planks fly off faster. dust puffs under every
 * plank it drops, where bridge_collapse's puffs under every other one. the
 * dust's test skips only a 100th plank, and these bridges have 36 and 53 */
static void falling_bridge_collapse(struct task_work *work)
{
    Angle ang;
    NJS_POINT3 *points;
    Float scale;
    NJS_POINT3 origin;
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    struct bridge_plank *planks;
    NJS_POINT3 *foot;
    struct task *child;
    struct task_work *cwork;
    struct plank_motion *motion;
    int i;
    int corner;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    Float post;

    planks = BRIDGE_VARS(work)->planks;
    points = BRIDGE_VARS(work)->object->model->points;
    scale = BRIDGE_VARS(work)->parts->scale;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    for (i = 1; i < BRIDGE_PLANK_COUNT(work) - 1; i++) {
        /* @hack redundant: post, set and never read, which leaves its
         * address unused in R3, from MOV #36,R3 at 0x0c9097d8; without it, the
         * function runs 4 bytes short, and 203 of 262 words differ */
        post = planks[i].post;
        corner = planks[i].corner[CORNER_FOOT_PLUS_Z];
        foot = &points[corner];
        njPushMatrix(NULL);
        njTranslate(NULL, foot->x, planks[i].height - 1.5F, 0.0F);
        origin = plank_origin;
        njCalcPoint(NULL, &origin, &pos);
        child = task_create(TASK_WORK | TASK_MOTION, 5, bridge_plank_step);
        if (child != NULL) {
            cwork = child->work;
            motion = (struct plank_motion *)child->motion;
            cwork->pos = pos;
            cwork->scl.x = scale;
            cwork->ang.x = planks[i].tilt * ANGLE_PER_DEGREE;
            cwork->ang.y = work->ang.y;
            PLANK_VARS(cwork)->model = BRIDGE_VARS(work)->parts->plank;
            PLANK_VARS(cwork)->texlist = BRIDGE_VARS(work)->parts->texlist;
            /* @hack redundant: ang, set again and never read, which keeps
             * points at @(4,R15) and scale at @(8,R15), stored at 0x0c909788
             * and 0x0c909794; without it, the two trade frame slots, and 4
             * of 262 words differ */
            ang = cwork->ang.y;
            /* @hack spelling: the whole wait written in each arm. SHC joins
             * the arms' tails at the LDS at 0x0c90988a, and the joined tail
             * reads the distance from R2 and COLLAPSE_SPREAD from FR13, at
             * 0x0c909896. with the wait written once after the if, the
             * distance takes R5, COLLAPSE_SPREAD loads at its use, and 114 of
             * 262 words differ */
            if (i < COLLAPSE_MIDDLE) {
                cwork->tick =
                    (int)((Float)(COLLAPSE_MIDDLE - i) *
                          (Float)(COLLAPSE_MIDDLE - i) * COLLAPSE_SPREAD) +
                    COLLAPSE_WAIT;
            } else {
                cwork->tick =
                    (int)((Float)(i - COLLAPSE_MIDDLE) *
                          (Float)(i - COLLAPSE_MIDDLE) * COLLAPSE_SPREAD) +
                    COLLAPSE_WAIT;
            }
            motion->speed.x = -0.2F;
            motion->speed.y = RANDOM(-1.5F, 0.5F);
            motion->speed.z = -0.2F;
            motion->spin_x = RANDOM(-7.5F, 7.5F) * ANGLE_PER_DEGREE;
            motion->spin_y = RANDOM(-2.5F, 2.5F) * ANGLE_PER_DEGREE;
        }
        if (i % 100) {
            speed = collapse_dust_speed;
            pos.y += 5.0F;
            pos.x -= 20.0F;
            dust_create(&pos, &speed, 4.5F);
        }
        njPopMatrix(1);
    }
    njPopMatrix(1);
}

void windy_sai_bridge_step(struct task *task)
{
    struct task_work *work = task->work;

    /* @hack spelling: task->work, read here and again for the collision, where
     * work holds it. the image keeps work on the stack for
     * enemy_alloc_record (0x0c909978) and loads task->work again at
     * 0x0c909996; with work in place of both reads, 35 of 36 words differ */
    BRIDGE_VARS(task->work)->parts = &windy_rope_bridge_parts;
    enemy_alloc_record(task, work);
    collision_init(task, sai_bridge_shapes, COLLISION_ITEM, 1, 1);
    task->work->collision->radius = BRIDGE_REACH;
    bridge_init(task);
    task->step = sai_bridge_step;
}

/* the sai stands on the floor below sai_place until a hit damages it. then
 * it bursts, the bridge breaks at two places, and the bridge sinks until it
 * destroys itself */
static void sai_bridge_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    NJS_POINT3 pos;

    if (edit_mode) {
        bridge_edit_draw(work);
    } else {
        switch (work->submode) {
        /* @hack redundant: each case's locals in a scope of its own, and the
         * hit's in scopes within it, where they share a 32-byte frame (ADD
         * #-32,R15 at 0x0c909a72); declared at the function's top, they take
         * 56 bytes, and 28 of 388 words differ */
        case SAI_WAIT: {
            struct angles slope;

            bridge_step_whole(task);
            if (enemy_take_hit(work, LOCKON_TARGET(task))) {
                work->submode++;
            } else {
                pos = sai_place;
                njPushMatrix(&_unit_matrix_);
                njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
                TURN_VIA(njRotateY, work->ang.y, ang);
                njCalcPoint(NULL, &pos, &pos);
                njPopMatrix(1);
                pos.y = ground_below(pos.x, pos.y + 10.0F, pos.z, &slope);
                njSetTexture(&e_sai_texlist);
                njPushMatrix(NULL);
                njTranslate(NULL, pos.x, pos.y, pos.z);
                TURN_VIA(njRotateZ, slope.z, ang);
                TURN_VIA(njRotateX, slope.x, ang);
                TURN_VIA(njRotateY, work->ang.y + 0x8000, ang);
                draw_tree(&sai_object);
                njPopMatrix(1);
                collision_register(work);
            }
        } break;
        case SAI_HIT: {
            int i;

            pos = sai_burst;
            njPushMatrix(&_unit_matrix_);
            njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
            TURN_VIA(njRotateY, work->ang.y, ang);
            njCalcPoint(NULL, &pos, &pos);
            njPopMatrix(1);
            explosion_create_at(pos.x, pos.y, pos.z, 1.4F);
            {
                /* enemy_break breaks the sai at work->pos, which holds the
                 * burst's place until it returns */
                NJS_POINT3 saved;

                saved.x = work->pos.x;
                saved.y = work->pos.y;
                saved.z = work->pos.z;
                work->pos.x = pos.x;
                work->pos.y = pos.y;
                work->pos.z = pos.z;
                enemy_break(work, sai_debris_models, &e_sai_texlist);
                work->pos.x = saved.x;
                work->pos.y = saved.y;
                work->pos.z = saved.z;
            }
            {
                NJS_POINT3 at;

                /* a thrown piece takes its x and z speeds at random. each
                 * lies between 0 and its parent's scl.x or scl.z */
                work->scl.x = 0.7F;
                work->scl.z = -0.7F;
                pos = break_place;
                for (i = 0; i < 2; i++) {
                    pos.x = i * BREAK_GAP;
                    njPushMatrix(&_unit_matrix_);
                    TURN_VIA(njRotateZ, work->ang.z, ang);
                    TURN_VIA(njRotateX, work->ang.x, ang);
                    TURN_VIA(njRotateY, work->ang.y, ang);
                    njCalcPoint(NULL, &pos, &at);
                    njPopMatrix(1);
                    at.x += work->pos.x;
                    at.y += work->pos.y;
                    at.z += work->pos.z;
                    windy_bridge_break_at(&at, TRUE, task);
                }
            }
            bridge_step_broken(task);
            work->submode++;
        } break;
        case SAI_BROKEN:
            task_step_children(task);
            bridge_step_broken(task);
            work->flags &= ~WORLD_TESTED;
            work->submode++;
            break;
        default:
            task_step_children(task);
            bridge_step_broken(task);
            work->flags &= ~WORLD_TESTED;
            work->pos.y -= 2.0F;
            work->ang.y += 15;
            if (work->submode++ > SINK_STEPS) {
                sai_bridge_end(task);
                placement_kill(task);
            }
            break;
        }
    }
}

void windy_stbrid_step(struct task *task)
{
    struct task_work *work = task->work;

    WORLD_MODEL(work)->model = &windy_stbrid_object;
    model_start_world(task);
}

/* destroy the pieces the two breaks made */
static void sai_bridge_end(struct task *task)
{
    struct task_work *work = task->work;

    enemy_end(task, work, LOCKON_TARGET(task));
    task_destroy_children(task);
}

void windy_yure_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        switch (work->mode) {
        case YURE_START:
            task->destructor = windy_yure_destroy;
            task->draw = windy_yure_draw;
            work->mode = YURE_RUN;
            break;
        case YURE_RUN:
            if (!placement_editing(task)) {
                YURE_VARS(work)->frame += work->scl.x;
            }
            if (placement_editing(task)) {
                njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- MOTION_SPEED");
            }
            windy_yure_draw(task);
            break;
        case YURE_STOP:
        case YURE_END:
            break;
        }
    }
}

static void windy_yure_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ACTION *action = &windy_yure_action;
    Angle ang;

    if (YURE_VARS(work)->frame >= action->motion->nbFrame) {
        YURE_VARS(work)->frame = 0.0F;
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_action(action, YURE_VARS(work)->frame);
    njPopMatrix(1);
}

static void windy_yure_destroy(struct task *task)
{
}

/* Chaos 4's lily pads. a pad floats at each of pad_places, and a stalk
 * sways beside each. a pad wobbles as a player lands on it. a record of
 * kind COLLISION_ENEMY, as Chaos 4's and the columns' are, can strike a
 * pad. struck as it floats, a pad sends up a column of water, and throws
 * player 0 off if a player stands on it. a struck pad sinks a while, then
 * rises again */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos4/column.h"
#include "b_chaos4/pads.h"
#include "b_chaos4/swamp.h"
#include "b_chaos4/textures.h"
#include "b_chaos4/util.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "enemy/enemy.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

enum pad_mode { PAD_START, PAD_FLOAT, PAD_SINK, PAD_RISE };

/* a pad floats at PAD_TOP, PAD_LIFT above its place on the swamp's
 * surface, where it starts. a struck pad moves by PAD_SINK_SPEED a step for
 * SINK_TIME steps, then rises back at PAD_RISE_SPEED a step */
#define PAD_LIFT 2.0F
#define PAD_RISE_SPEED 0.32F
#define PAD_TOP (SWAMP_Y + PAD_LIFT)
#define PAD_SINK_SPEED (-0.55F)
#define SINK_TIME 180

/* a pad's wobble. a strike on a floating pad sets its turn a step about x
 * and z at random, up to STRIKE_WOBBLE, about 10 degrees. a player's
 * landing sets it up to LAND_WOBBLE, about 2 degrees. for WOBBLE_TIME
 * steps, the turn keeps WOBBLE_DRAG of itself each step and loses
 * WOBBLE_SPRING of the pad's tilt, which pulls the pad back level. then the
 * pad stands level */
#define STRIKE_WOBBLE 1820.0F
#define LAND_WOBBLE 364.0F
#define WOBBLE_TIME 180
#define WOBBLE_DRAG 0.95F
#define WOBBLE_SPRING 0.01F

#define SE_PAD_SINK 0x172 /* the sound of a strike on a floating pad */
/* the sound of a player's landing on a pad, and of a pad back up */
#define SE_PAD_LAND 0x173

/* the vibration pack's power and frequency as a pad throws player 0 off.
 * vibration_start clamps the power to 4 */
#define THROW_POWER 11
#define SHAKE_FREQ 30

/* the radius of a pad's cylinder. a stalk stands at least STALK_GAP clear
 * of every pad's cylinder, and up to STALK_SPREAD farther from its own
 * pad's */
#define PAD_RADIUS 30.79986F
#define STALK_GAP 10.0F
#define STALK_CLEARANCE (PAD_RADIUS + STALK_GAP)
#define STALK_SPREAD 10.0F
/* the reach pad_create gives a pad's collision record */
#define PAD_REACH 40.0F
/* only a point of the waves within NEAREST_LIMIT of a stalk in x and z can
 * be the point it rides */
#define NEAREST_LIMIT 1000.0F
/* the frames a stalk's motion runs a step. a stalk stands at its point's
 * offset from SWAMP_Y, the swamp's surface */
#define STALK_PACE 0.5F

#define NO_RIDER 6 /* a pad's rider while no player touches it */

/* a pad's record, at its task's extra */
struct pad {
    NJS_VECTOR wobble; /* the turn its wobble adds to its angles a step */
    /* the player on the pad: 0 while any player touches it, which a strike
     * takes as player 0, else NO_RIDER. a sinking pad keeps its last rider
     * until it rises */
    Uint16 rider;
    Sint16 wobble_time; /* the steps its wobble has left */
    Uint8 index; /* its place's index in pad_places */
};

struct pad_vars {
    Uint8 unk_00[4];
    Sint32 timer; /* the steps a struck pad has left to sink */
};

#define PAD_VARS(work) ((struct pad_vars *)&(work)->vars)

struct stalk_vars {
    Float frame; /* its motion's frame */
    Sint32 point; /* the index of the waves' point it rides */
};

#define STALK_VARS(work) ((struct stalk_vars *)&(work)->vars)

static void pad_create(NJS_POINT3 *pos, Uint8 index);
static void pad_step(struct task *task);
static void pad_draw(struct task *task);
static void pad_destroy(struct task *task);
static void stalk_create(NJS_POINT3 *center);
static void stalk_place(NJS_POINT3 *center, NJS_POINT3 *pos);
static int stalk_nearest_point(NJS_POINT3 *pos);
static void stalk_step(struct task *task);
static void stalk_draw(struct task *task);

static NJS_MATERIAL pad_materials[2] = {
#include "b_chaos4/pads/pad_materials.inc"
};

static Sint16 pad_meshes0[18] = {
#include "b_chaos4/pads/pad_meshes0.inc"
};

static Sint16 pad_meshes1[36] = {
#include "b_chaos4/pads/pad_meshes1.inc"
};

static NJS_COLOR pad_uvs0[16] = {
#include "b_chaos4/pads/pad_uvs0.inc"
};

static NJS_COLOR pad_uvs1[32] = {
#include "b_chaos4/pads/pad_uvs1.inc"
};

static NJS_MESHSET pad_meshsets[2] = {
#include "b_chaos4/pads/pad_meshsets.inc"
};

static NJS_POINT3 pad_points[29] = {
#include "b_chaos4/pads/pad_points.inc"
};

static NJS_VECTOR pad_normals[29] = {
#include "b_chaos4/pads/pad_normals.inc"
};

static NJS_MODEL pad_model = {
#include "b_chaos4/pads/pad_model.inc"
};

static NJS_OBJECT pad_object = {
#include "b_chaos4/pads/pad_object.inc"
};

/* a pad's upright cylinder. it pushes every group, and no group pushes
 * it */
static struct collision_shape pad_shape[1] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, -25.3F, 0.0F},
     PAD_RADIUS,
     30.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

/* the speed a struck pad throws player 0 off with, back and up in the
 * frame player 0 faces. player_turn_to_world turns it into the world in
 * place, so each throw after the first turns the last throw's world speed
 * again */
static NJS_VECTOR pad_throw = {-1.2F, 1.2F, 0.0F};

NJS_POINT3 pad_places[PADS] = {
#include "b_chaos4/pads/pad_places.inc"
};

/* the stalk's model, a hidden root over five nodes, and its motion, which
 * turns the nodes through 60 frames */
static NJS_MATERIAL stalk_node5_materials[2] = {
#include "b_chaos4/pads/stalk_node5_materials.inc"
};

static Sint16 stalk_node5_meshes0[18] = {
#include "b_chaos4/pads/stalk_node5_meshes0.inc"
};

static Sint16 stalk_node5_meshes1[10] = {
#include "b_chaos4/pads/stalk_node5_meshes1.inc"
};

static NJS_COLOR stalk_node5_uvs0[16] = {
#include "b_chaos4/pads/stalk_node5_uvs0.inc"
};

static NJS_COLOR stalk_node5_uvs1[8] = {
#include "b_chaos4/pads/stalk_node5_uvs1.inc"
};

static NJS_MESHSET stalk_node5_meshsets[2] = {
#include "b_chaos4/pads/stalk_node5_meshsets.inc"
};

static NJS_POINT3 stalk_node5_points[17] = {
#include "b_chaos4/pads/stalk_node5_points.inc"
};

static NJS_VECTOR stalk_node5_normals[17] = {
#include "b_chaos4/pads/stalk_node5_normals.inc"
};

static NJS_MODEL stalk_node5_model = {
#include "b_chaos4/pads/stalk_node5_model.inc"
};

static NJS_OBJECT stalk_node5 = {
#include "b_chaos4/pads/stalk_node5.inc"
};

static NJS_MATERIAL stalk_node4_materials[2] = {
#include "b_chaos4/pads/stalk_node4_materials.inc"
};

static Sint16 stalk_node4_meshes0[18] = {
#include "b_chaos4/pads/stalk_node4_meshes0.inc"
};

static Sint16 stalk_node4_meshes1[10] = {
#include "b_chaos4/pads/stalk_node4_meshes1.inc"
};

static NJS_COLOR stalk_node4_uvs0[16] = {
#include "b_chaos4/pads/stalk_node4_uvs0.inc"
};

static NJS_COLOR stalk_node4_uvs1[8] = {
#include "b_chaos4/pads/stalk_node4_uvs1.inc"
};

static NJS_MESHSET stalk_node4_meshsets[2] = {
#include "b_chaos4/pads/stalk_node4_meshsets.inc"
};

static NJS_POINT3 stalk_node4_points[17] = {
#include "b_chaos4/pads/stalk_node4_points.inc"
};

static NJS_VECTOR stalk_node4_normals[17] = {
#include "b_chaos4/pads/stalk_node4_normals.inc"
};

static NJS_MODEL stalk_node4_model = {
#include "b_chaos4/pads/stalk_node4_model.inc"
};

static NJS_OBJECT stalk_node4 = {
#include "b_chaos4/pads/stalk_node4.inc"
};

static NJS_MATERIAL stalk_node3_materials[2] = {
#include "b_chaos4/pads/stalk_node3_materials.inc"
};

static Sint16 stalk_node3_meshes0[18] = {
#include "b_chaos4/pads/stalk_node3_meshes0.inc"
};

static Sint16 stalk_node3_meshes1[12] = {
#include "b_chaos4/pads/stalk_node3_meshes1.inc"
};

static NJS_COLOR stalk_node3_uvs0[16] = {
#include "b_chaos4/pads/stalk_node3_uvs0.inc"
};

static NJS_COLOR stalk_node3_uvs1[10] = {
#include "b_chaos4/pads/stalk_node3_uvs1.inc"
};

static NJS_MESHSET stalk_node3_meshsets[2] = {
#include "b_chaos4/pads/stalk_node3_meshsets.inc"
};

static NJS_POINT3 stalk_node3_points[18] = {
#include "b_chaos4/pads/stalk_node3_points.inc"
};

static NJS_VECTOR stalk_node3_normals[18] = {
#include "b_chaos4/pads/stalk_node3_normals.inc"
};

static NJS_MODEL stalk_node3_model = {
#include "b_chaos4/pads/stalk_node3_model.inc"
};

static NJS_OBJECT stalk_node3 = {
#include "b_chaos4/pads/stalk_node3.inc"
};

static NJS_MATERIAL stalk_node2_materials[2] = {
#include "b_chaos4/pads/stalk_node2_materials.inc"
};

static Sint16 stalk_node2_meshes0[18] = {
#include "b_chaos4/pads/stalk_node2_meshes0.inc"
};

static Sint16 stalk_node2_meshes1[10] = {
#include "b_chaos4/pads/stalk_node2_meshes1.inc"
};

static NJS_COLOR stalk_node2_uvs0[16] = {
#include "b_chaos4/pads/stalk_node2_uvs0.inc"
};

static NJS_COLOR stalk_node2_uvs1[8] = {
#include "b_chaos4/pads/stalk_node2_uvs1.inc"
};

static NJS_MESHSET stalk_node2_meshsets[2] = {
#include "b_chaos4/pads/stalk_node2_meshsets.inc"
};

static NJS_POINT3 stalk_node2_points[17] = {
#include "b_chaos4/pads/stalk_node2_points.inc"
};

static NJS_VECTOR stalk_node2_normals[17] = {
#include "b_chaos4/pads/stalk_node2_normals.inc"
};

static NJS_MODEL stalk_node2_model = {
#include "b_chaos4/pads/stalk_node2_model.inc"
};

static NJS_OBJECT stalk_node2 = {
#include "b_chaos4/pads/stalk_node2.inc"
};

static NJS_MATERIAL stalk_node1_materials[2] = {
#include "b_chaos4/pads/stalk_node1_materials.inc"
};

static Sint16 stalk_node1_meshes0[18] = {
#include "b_chaos4/pads/stalk_node1_meshes0.inc"
};

static Sint16 stalk_node1_meshes1[10] = {
#include "b_chaos4/pads/stalk_node1_meshes1.inc"
};

static NJS_COLOR stalk_node1_uvs0[16] = {
#include "b_chaos4/pads/stalk_node1_uvs0.inc"
};

static NJS_COLOR stalk_node1_uvs1[8] = {
#include "b_chaos4/pads/stalk_node1_uvs1.inc"
};

static NJS_MESHSET stalk_node1_meshsets[2] = {
#include "b_chaos4/pads/stalk_node1_meshsets.inc"
};

static NJS_POINT3 stalk_node1_points[17] = {
#include "b_chaos4/pads/stalk_node1_points.inc"
};

static NJS_VECTOR stalk_node1_normals[17] = {
#include "b_chaos4/pads/stalk_node1_normals.inc"
};

static NJS_MODEL stalk_node1_model = {
#include "b_chaos4/pads/stalk_node1_model.inc"
};

static NJS_OBJECT stalk_node1 = {
#include "b_chaos4/pads/stalk_node1.inc"
};

static NJS_OBJECT stalk_node0 = {
#include "b_chaos4/pads/stalk_node0.inc"
};

static NJS_MKEY_A stalk_ang1[60] = {
#include "b_chaos4/pads/stalk_ang1.inc"
};

static NJS_MKEY_A stalk_ang2[60] = {
#include "b_chaos4/pads/stalk_ang2.inc"
};

static NJS_MKEY_A stalk_ang3[60] = {
#include "b_chaos4/pads/stalk_ang3.inc"
};

static NJS_MKEY_A stalk_ang4[60] = {
#include "b_chaos4/pads/stalk_ang4.inc"
};

static NJS_MKEY_A stalk_ang5[60] = {
#include "b_chaos4/pads/stalk_ang5.inc"
};

static NJS_MDATA2 stalk_mdata[6] = {
#include "b_chaos4/pads/stalk_mdata.inc"
};

static NJS_MOTION stalk_motion = {
#include "b_chaos4/pads/stalk_motion.inc"
};

static NJS_ACTION stalk_action[1] = {
#include "b_chaos4/pads/stalk_action.inc"
};

Uint8 pad_stood_on;

/* make a pad PAD_LIFT above pos, for place index */
static void pad_create(NJS_POINT3 *pos, Uint8 index)
{
    struct task *task;
    struct task_work *work;
    struct pad *pad;

    /* @hack spelling: the task tested in the statement that stores it,
     * which copies it to R14 in the slot of the test's branch (BT/S at
     * 0x0c9065fc); stored and then tested, 3 of 75 words differ */
    if ((task = task_create(TASK_WORK, 3, pad_step)) != NULL) {
        work = task->work;
        pad = heap_calloc(1, sizeof(struct pad));
        enemy_alloc_record(task, work);
        task->extra = pad;
        VECTOR_COPY(&work->pos, pos);
        work->pos.y += PAD_LIFT;
        pad->index = index;
        collision_init(task, pad_shape, COLLISION_OBJECT_MUTUAL, 1, 1);
        work->collision->radius = PAD_REACH;
        task->step = pad_step;
        task->draw = pad_draw;
        task->destructor = pad_destroy;
    }
}

static void pad_step(struct task *task)
{
    struct task_work *work = task->work;
    struct pad *pad = task->extra;
    NJS_VECTOR spring;
    struct collision_work *hit;
    struct collision_shape *shape;

    switch (work->mode) {
    case PAD_START:
        work->mode = PAD_FLOAT;
        pad->rider = NO_RIDER;
        work->ang.y = RANDOM_ANGLE();
        break;
    case PAD_FLOAT:
    case PAD_RISE:
        if ((work->collision->flags & COLLISION_HIT) &&
            work->collision->shapes->hit->kind == COLLISION_ENEMY) {
            if (work->mode == PAD_FLOAT) {
                column_create(&work->pos, NULL);
                pad->wobble.x = rand() * (STRIKE_WOBBLE * RAND_SCALE);
                pad->wobble.z = rand() * (STRIKE_WOBBLE * RAND_SCALE);
                pad->wobble.y = 0.0F;
                pad->wobble_time = WOBBLE_TIME;
                swamp_ripple_add(&work->pos);
                sound_play(SE_PAD_SINK, 0, 0, 0);
                if (pad->rider == 0) {
                    player_turn_to_world(player_work[0], &pad_throw);
                    player_set_speed(0, pad_throw.x, pad_throw.y, pad_throw.z);
                    vibration_start(0, THROW_POWER, SHAKE_FREQ, 1);
                }
            }
            PAD_VARS(work)->timer = SINK_TIME;
            work->mode = PAD_SINK;
        }
        if (work->flags & COLLISION_TOUCHED) {
            shape = work->collision->shapes;
            hit = shape->hit;
            if (hit->kind == COLLISION_PLAYER) {
                pad_stood_on = pad->index;
                if (pad->rider == NO_RIDER) {
                    player_clear_speed(NULL, player_get(0));
                    pad->wobble.x = rand() * (LAND_WOBBLE * RAND_SCALE);
                    pad->wobble.z = rand() * (LAND_WOBBLE * RAND_SCALE);
                    pad->wobble.y = 0.0F;
                    pad->wobble_time = WOBBLE_TIME;
                    sound_play(SE_PAD_LAND, 0, 0, 0);
                }
                pad->rider = 0;
            } else {
                pad->rider = NO_RIDER;
            }
        } else {
            pad->rider = NO_RIDER;
        }
        /* @hack spelling: the rise added in the test, which compares the sum
         * still in FR2 and stores it in the slot of the test's branch (BF/S
         * at 0x0c90689c); added ahead of it, SHC reads the height back from
         * the work and 23 of 326 words differ */
        if ((work->pos.y += PAD_RISE_SPEED) > PAD_TOP) {
            work->pos.y = PAD_TOP;
            if (work->mode == PAD_RISE) {
                sound_play(SE_PAD_LAND, 0, 0, 0);
                work->mode = PAD_FLOAT;
            }
        }
        break;
    case PAD_SINK:
        work->pos.y += PAD_SINK_SPEED;
        if (--PAD_VARS(work)->timer <= 0) {
            pad->rider = NO_RIDER;
            work->mode = PAD_RISE;
        }
        break;
    }
    if (pad->wobble_time > 0) {
        VECTOR_SCALE(&pad->wobble, &pad->wobble, WOBBLE_DRAG);
        VECTOR_SCALE(&spring, &work->ang, WOBBLE_SPRING);
        VECTOR_DIFF(&pad->wobble, &pad->wobble, &spring);
        pad->wobble.y = 0.0F;
        VECTOR_SUM(&work->ang, &work->ang, &pad->wobble);
        if (--pad->wobble_time <= 0) {
            pad->wobble.x = pad->wobble.y = pad->wobble.z = 0.0F;
            work->ang.x = work->ang.z = 0;
        }
    }
    collision_register(work);
    pad_draw(task);
}

static void pad_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        chaos4_material_reset();
        njSetTexture(&chaos4_object_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&pad_object);
        njPopMatrix(1);
    }
}

static void pad_destroy(struct task *task)
{
}

/* make a stalk near center, clear of every pad, with a random heading. it
 * rides the waves' point nearest it */
static void stalk_create(NJS_POINT3 *center)
{
    struct task *task;
    struct task_work *work;

    /* @hack spelling: the task tested in the statement that stores it,
     * which copies it to R13 in the slot of the test's branch (BT/S at
     * 0x0c906ab6); stored and then tested, 3 of 62 words differ */
    if ((task = task_create(TASK_WORK, 3, stalk_step)) != NULL) {
        work = task->work;
        enemy_alloc_record(task, work);
        stalk_place(center, &work->pos);
        STALK_VARS(work)->point = stalk_nearest_point(&work->pos);
        work->ang.y = RANDOM_ANGLE();
        STALK_VARS(work)->frame = (Float)stalk_action->motion->nbFrame;
        task->step = stalk_step;
        task->draw = stalk_draw;
    }
}

/* pick pos at random, STALK_CLEARANCE to STALK_CLEARANCE + STALK_SPREAD
 * from center in x and z, until it lies at least STALK_CLEARANCE from every
 * pad's place */
static void stalk_place(NJS_POINT3 *center, NJS_POINT3 *pos)
{
    Float ang;
    Float radius;
    int i;
    Uint8 clear;

    do {
        ang = RANDOM_ANGLE();
        radius = STALK_CLEARANCE + (rand() * (STALK_SPREAD * RAND_SCALE));
        VECTOR_COPY(pos, center);
        pos->x += radius * njSin((Angle)ang);
        pos->z += radius * njCos((Angle)ang);
        clear = TRUE;
        for (i = 0; i < PADS; i++) {
            if (njSqrt(SQUARE(pos->x - pad_places[i].x) +
                       SQUARE(pos->z - pad_places[i].z)) < STALK_CLEARANCE) {
                clear = FALSE;
                break;
            }
        }
    } while (clear != TRUE);
}

/* the index of the waves' point nearest pos in x and z. 0 without the
 * waves, or with no point within NEAREST_LIMIT */
static int stalk_nearest_point(NJS_POINT3 *pos)
{
    struct swamp *swamp = swamp_current;
    NJS_MODEL *model;
    NJS_POINT3 *points;
    NJS_VECTOR to;
    Float nearest;
    Float distance;
    int point;
    int i;

    if (swamp) {
        model = swamp->waves->model;
        points = model->points;
        nearest = NEAREST_LIMIT;
        point = 0;
        for (i = 0; i < model->nbPoint; i++) {
            VECTOR_DIFF(&to, pos, &points[i]);
            distance = njSqrt(SQUARE(to.x) + SQUARE(to.z));
            if (distance < nearest) {
                point = i;
                nearest = distance;
            }
        }
        return point;
    }
    return 0;
}

/* a stalk's motion runs STALK_PACE frames a step, from frame 0 again once
 * it reaches its last. while the waves exist, the stalk stands at its
 * point's offset from SWAMP_Y */
static void stalk_step(struct task *task)
{
    struct task_work *work = task->work;
    struct swamp *swamp;

    /* @hack spelling: the frame stepped in the test, which compares the sum
     * still in FR2 (FADD at 0x0c906d2e, FCMP/GT at 0x0c906d4a); stepped
     * ahead of it, SHC reads the frame back from the work, stalk_step runs 2
     * bytes long and 24 of 40 words differ */
    if ((STALK_VARS(work)->frame += STALK_PACE) >=
        (Float)stalk_action->motion->nbFrame - 1.0F) {
        STALK_VARS(work)->frame = 0.0F;
    }
    if ((swamp = swamp_current) != NULL) {
        work->pos.y = swamp->offsets[STALK_VARS(work)->point] + SWAMP_Y;
    }
    stalk_draw(task);
}

static void stalk_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        chaos4_material_reset();
        njSetTexture(&chaos4_object_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_action(stalk_action, STALK_VARS(work)->frame);
        njPopMatrix(1);
    }
}

void pads_create(void)
{
    int i;
    int j;
    int stalks;

    for (i = 0; i < PADS; i++) {
        pad_create(&pad_places[i], i);
        /* @bug logic: the count truncates rand() * RAND_SCALE, below 1,
         * to 0 (FTRC at 0x0c906e60) before it adds 1 (0x0c906e64), so each
         * pad gets one stalk */
        stalks = (int)(rand() * RAND_SCALE) + 1;
        for (j = 0; j < stalks; j++) {
            stalk_create(&pad_places[i]);
        }
    }
}

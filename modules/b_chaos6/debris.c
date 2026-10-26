/* Chaos 6's debris: pieces thrown out and up from round the boss. each
 * tumbles as it falls, and its task ends once it drops below the deck */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos6/boss.h"
#include "b_chaos6/debris.h"
#include "b_chaos6/textures.h"
#include "draw/motion.h"
#include "draw/tree.h"
#include "system/task.h"
#include "system/util.h"

/* debris_pieces holds two sets of pieces. chaos6_debris_scatter's kind 0
 * throws the last four, and every other kind the first three */
#define FIRST_PIECES 3
#define LAST_PIECES 4
#define PIECES (FIRST_PIECES + LAST_PIECES)

/* how far a piece turns about x and about z a step */
#define DEBRIS_SPIN 512
/* how much a piece's upward speed drops a step: 9.8 / 360, written as a
 * decimal. SHC folds 9.8F / 360.0F to 0x3cdf0122, one below the image's
 * 0x3cdf0123 */
#define DEBRIS_FALL 0.027222222F
/* the deck's height. a piece's task kills itself once the piece is below
 * it */
#define DEBRIS_BOTTOM 744.0F

struct debris_vars {
    Uint8 unk_00[4];
    Sint8 piece; /* its index in debris_pieces */
};

#define VARS(work) ((struct debris_vars *)&(work)->vars)

/* a piece starts debris_reach from the boss, along its heading tilted up by
 * debris_tilt, and debris_height above that. it moves at debris_speed a step
 * along the same tilted heading. each piece takes up to half again of the
 * speed, the reach and the height, at random. it takes up to 30 degrees more
 * of the tilt the same way */
#pragma section HEAD
static Float debris_speed = 0.75F;
static Angle debris_tilt = 0x1400;
static Sint32 dat_0c900338 = 0;
static Float debris_reach = 30.0F;
static Float debris_height = 10.0F;
#pragma section

static NJS_MATERIAL chaos6_debris_0_materials[1] = {
#include "b_chaos6/debris/chaos6_debris_0_materials.inc"
};

static Sint16 chaos6_debris_0_meshes0[64] = {
#include "b_chaos6/debris/chaos6_debris_0_meshes0.inc"
};

static NJS_COLOR chaos6_debris_0_uvs0[48] = {
#include "b_chaos6/debris/chaos6_debris_0_uvs0.inc"
};

static NJS_MESHSET chaos6_debris_0_meshsets[1] = {
#include "b_chaos6/debris/chaos6_debris_0_meshsets.inc"
};

static NJS_POINT3 chaos6_debris_0_points[10] = {
#include "b_chaos6/debris/chaos6_debris_0_points.inc"
};

static NJS_VECTOR chaos6_debris_0_normals[10] = {
#include "b_chaos6/debris/chaos6_debris_0_normals.inc"
};

static NJS_MODEL chaos6_debris_0_model = {
#include "b_chaos6/debris/chaos6_debris_0_model.inc"
};

static NJS_OBJECT chaos6_debris_0_object = {
#include "b_chaos6/debris/chaos6_debris_0_object.inc"
};

static NJS_MATERIAL chaos6_debris_1_materials[1] = {
#include "b_chaos6/debris/chaos6_debris_1_materials.inc"
};

static Sint16 chaos6_debris_1_meshes0[96] = {
#include "b_chaos6/debris/chaos6_debris_1_meshes0.inc"
};

static NJS_COLOR chaos6_debris_1_uvs0[72] = {
#include "b_chaos6/debris/chaos6_debris_1_uvs0.inc"
};

static NJS_MESHSET chaos6_debris_1_meshsets[1] = {
#include "b_chaos6/debris/chaos6_debris_1_meshsets.inc"
};

static NJS_POINT3 chaos6_debris_1_points[14] = {
#include "b_chaos6/debris/chaos6_debris_1_points.inc"
};

static NJS_VECTOR chaos6_debris_1_normals[14] = {
#include "b_chaos6/debris/chaos6_debris_1_normals.inc"
};

static NJS_MODEL chaos6_debris_1_model = {
#include "b_chaos6/debris/chaos6_debris_1_model.inc"
};

static NJS_OBJECT chaos6_debris_1_object = {
#include "b_chaos6/debris/chaos6_debris_1_object.inc"
};

static NJS_MATERIAL chaos6_debris_2_materials[1] = {
#include "b_chaos6/debris/chaos6_debris_2_materials.inc"
};

static Sint16 chaos6_debris_2_meshes0[96] = {
#include "b_chaos6/debris/chaos6_debris_2_meshes0.inc"
};

static NJS_COLOR chaos6_debris_2_uvs0[72] = {
#include "b_chaos6/debris/chaos6_debris_2_uvs0.inc"
};

static NJS_MESHSET chaos6_debris_2_meshsets[1] = {
#include "b_chaos6/debris/chaos6_debris_2_meshsets.inc"
};

static NJS_POINT3 chaos6_debris_2_points[14] = {
#include "b_chaos6/debris/chaos6_debris_2_points.inc"
};

static NJS_VECTOR chaos6_debris_2_normals[14] = {
#include "b_chaos6/debris/chaos6_debris_2_normals.inc"
};

static NJS_MODEL chaos6_debris_2_model = {
#include "b_chaos6/debris/chaos6_debris_2_model.inc"
};

static NJS_OBJECT chaos6_debris_2_object = {
#include "b_chaos6/debris/chaos6_debris_2_object.inc"
};

static NJS_MATERIAL chaos6_debris_3_materials[1] = {
#include "b_chaos6/debris/chaos6_debris_3_materials.inc"
};

static Sint16 chaos6_debris_3_meshes0[76] = {
#include "b_chaos6/debris/chaos6_debris_3_meshes0.inc"
};

static NJS_COLOR chaos6_debris_3_uvs0[57] = {
#include "b_chaos6/debris/chaos6_debris_3_uvs0.inc"
};

static NJS_MESHSET chaos6_debris_3_meshsets[1] = {
#include "b_chaos6/debris/chaos6_debris_3_meshsets.inc"
};

static NJS_POINT3 chaos6_debris_3_points[16] = {
#include "b_chaos6/debris/chaos6_debris_3_points.inc"
};

static NJS_VECTOR chaos6_debris_3_normals[16] = {
#include "b_chaos6/debris/chaos6_debris_3_normals.inc"
};

static NJS_MODEL chaos6_debris_3_model = {
#include "b_chaos6/debris/chaos6_debris_3_model.inc"
};

static NJS_OBJECT chaos6_debris_3_object = {
#include "b_chaos6/debris/chaos6_debris_3_object.inc"
};

static NJS_MATERIAL chaos6_debris_4_materials[1] = {
#include "b_chaos6/debris/chaos6_debris_4_materials.inc"
};

static Sint16 chaos6_debris_4_meshes0[60] = {
#include "b_chaos6/debris/chaos6_debris_4_meshes0.inc"
};

static NJS_COLOR chaos6_debris_4_uvs0[45] = {
#include "b_chaos6/debris/chaos6_debris_4_uvs0.inc"
};

static NJS_MESHSET chaos6_debris_4_meshsets[1] = {
#include "b_chaos6/debris/chaos6_debris_4_meshsets.inc"
};

static NJS_POINT3 chaos6_debris_4_points[13] = {
#include "b_chaos6/debris/chaos6_debris_4_points.inc"
};

static NJS_VECTOR chaos6_debris_4_normals[13] = {
#include "b_chaos6/debris/chaos6_debris_4_normals.inc"
};

static NJS_MODEL chaos6_debris_4_model = {
#include "b_chaos6/debris/chaos6_debris_4_model.inc"
};

static NJS_OBJECT chaos6_debris_4_object = {
#include "b_chaos6/debris/chaos6_debris_4_object.inc"
};

static NJS_MATERIAL chaos6_debris_5_materials[1] = {
#include "b_chaos6/debris/chaos6_debris_5_materials.inc"
};

static Sint16 chaos6_debris_5_meshes0[37] = {
#include "b_chaos6/debris/chaos6_debris_5_meshes0.inc"
};

static NJS_COLOR chaos6_debris_5_uvs0[28] = {
#include "b_chaos6/debris/chaos6_debris_5_uvs0.inc"
};

static NJS_MESHSET chaos6_debris_5_meshsets[1] = {
#include "b_chaos6/debris/chaos6_debris_5_meshsets.inc"
};

static NJS_POINT3 chaos6_debris_5_points[10] = {
#include "b_chaos6/debris/chaos6_debris_5_points.inc"
};

static NJS_VECTOR chaos6_debris_5_normals[10] = {
#include "b_chaos6/debris/chaos6_debris_5_normals.inc"
};

static NJS_MODEL chaos6_debris_5_model = {
#include "b_chaos6/debris/chaos6_debris_5_model.inc"
};

static NJS_OBJECT chaos6_debris_5_object = {
#include "b_chaos6/debris/chaos6_debris_5_object.inc"
};

static NJS_MATERIAL chaos6_debris_6_materials[1] = {
#include "b_chaos6/debris/chaos6_debris_6_materials.inc"
};

static Sint16 chaos6_debris_6_meshes0[12] = {
#include "b_chaos6/debris/chaos6_debris_6_meshes0.inc"
};

static NJS_COLOR chaos6_debris_6_uvs0[9] = {
#include "b_chaos6/debris/chaos6_debris_6_uvs0.inc"
};

static NJS_MESHSET chaos6_debris_6_meshsets[1] = {
#include "b_chaos6/debris/chaos6_debris_6_meshsets.inc"
};

static NJS_POINT3 chaos6_debris_6_points[5] = {
#include "b_chaos6/debris/chaos6_debris_6_points.inc"
};

static NJS_VECTOR chaos6_debris_6_normals[5] = {
#include "b_chaos6/debris/chaos6_debris_6_normals.inc"
};

static NJS_MODEL chaos6_debris_6_model = {
#include "b_chaos6/debris/chaos6_debris_6_model.inc"
};

static NJS_OBJECT chaos6_debris_6_object = {
#include "b_chaos6/debris/chaos6_debris_6_object.inc"
};

static NJS_OBJECT *debris_pieces[PIECES] = {
    &chaos6_debris_0_object, &chaos6_debris_1_object, &chaos6_debris_2_object,
    &chaos6_debris_3_object, &chaos6_debris_4_object, &chaos6_debris_5_object,
    &chaos6_debris_6_object,
};

static void chaos6_debris_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    Float scale;

    njSetTexture(&chaos6_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    scale = work->scl.x;
    njScale(NULL, scale, scale, scale);
    draw_tree(debris_pieces[VARS(work)->piece]);
    njPopMatrix(1);
}

static void chaos6_debris_run(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    work->ang.x += DEBRIS_SPIN;
    work->ang.z += DEBRIS_SPIN;
    motion->speed.y -= DEBRIS_FALL;
    njAddVector(&work->pos, &motion->speed);
    if (work->pos.y < DEBRIS_BOTTOM) {
        task_kill(task);
    } else {
        chaos6_debris_draw(task);
    }
}

/* the piece's first step: place it out from the boss along work->ang.y,
 * tilted up, and set its speed along the same heading. it tumbles from
 * random angles about x and z, and its size is from 1 to 1.5 at random */
static void chaos6_debris_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    NJS_VECTOR out;
    Angle ang;

    task->step = chaos6_debris_run;
    task->draw = chaos6_debris_draw;
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, work->ang.y, ang);
    /* TURN reads its angle twice, so rand runs twice: TURN's test of the
     * angle calls it at 0x0c905f42, and the turn at 0x0c905f5e */
    TURN(njRotateZ, debris_tilt + (Angle)(rand() * (1.0F / 6.0F)));
    out.y = out.z = 0.0F;
    out.x = RANDOM(1.0F, 1.5F) * debris_reach;
    njCalcVector(NULL, &out, &work->pos);
    njAddVector(&work->pos, &boss_work->pos);
    work->pos.y += RANDOM(1.0F, 1.5F) * debris_height;
    out.x = RANDOM(1.0F, 1.5F) * debris_speed;
    njCalcVector(NULL, &out, &motion->speed);
    njPopMatrix(1);
    work->ang.x = RANDOM_ANGLE();
    work->ang.z = RANDOM_ANGLE();
    work->scl.x = RANDOM(1.0F, 1.5F);
}

void chaos6_debris_scatter(int kind, int count)
{
    struct task *task;
    struct task_work *work;
    Float spacing = 360.0F / count;
    Float heading;
    int i;

    for (i = 0, heading = 0.0F; i < count; i++, heading += spacing) {
        task = task_create(TASK_WORK | TASK_MOTION, 2, chaos6_debris_start);
        if (task != NULL) {
            work = task->work;
            if (kind == 0) {
                VARS(work)->piece = (Sint8)RANDOM(FIRST_PIECES, PIECES);
            } else {
                VARS(work)->piece =
                    (Sint8)(rand() * (FIRST_PIECES * RAND_SCALE));
            }
            work->ang.y = (Angle)(heading * ANGLE_PER_DEGREE);
        }
    }
}

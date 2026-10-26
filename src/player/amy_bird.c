/* the bird that circles over Amy */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "event/play.h"
#include "main.h"
#include "player/amy.h"
#include "player/amy_bird.h"
#include "player/player.h"
#include "player/textures.h"
#include "stage/stage.h"
#include "stg03/mirror.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the stage_act that ends amy_bird_create's list of stages, which no act
 * has */
#define BIRD_STAGES_END 0x7fff

/* the circle the bird flies round player 0: its radius, and its height over
 * her */
#define BIRD_RADIUS 3.0F
#define BIRD_HEIGHT 12.0F
/* the bird jumps to its place on the circle from this far away or more,
 * and eases there from nearer */
#define BIRD_JUMP_DISTANCE 200.0F
/* as she takes a drum, the bird flies to the point this far up her own y
 * axis */
#define PERCH_HEIGHT 7.0F

/* the bird keeps its angle round player 0 in work->ang.x, and its bob's
 * phase in work->ang.z */
struct bird_vars {
    Float frame; /* the frame of bird_flap_action it shows */
};

#define VARS(work) ((struct bird_vars *)&(work)->vars)

/* the bird's model, textured from amy_texlist's ld_ entries. each node's
 * data follows its sibling's and then its child's */
static NJS_MATERIAL bird_wing_tip_minus_z_materials[1] = {
#include "player/amy_bird/bird_wing_tip_minus_z_materials.inc"
};

static Sint16 bird_wing_tip_minus_z_meshes0[11] = {
#include "player/amy_bird/bird_wing_tip_minus_z_meshes0.inc"
};

static NJS_COLOR bird_wing_tip_minus_z_uvs0[9] = {
#include "player/amy_bird/bird_wing_tip_minus_z_uvs0.inc"
};

static NJS_MESHSET bird_wing_tip_minus_z_meshsets[1] = {
#include "player/amy_bird/bird_wing_tip_minus_z_meshsets.inc"
};

static NJS_POINT3 bird_wing_tip_minus_z_points[7] = {
#include "player/amy_bird/bird_wing_tip_minus_z_points.inc"
};

static NJS_VECTOR bird_wing_tip_minus_z_normals[7] = {
#include "player/amy_bird/bird_wing_tip_minus_z_normals.inc"
};

static NJS_MODEL bird_wing_tip_minus_z_model = {
#include "player/amy_bird/bird_wing_tip_minus_z_model.inc"
};

static NJS_OBJECT bird_wing_tip_minus_z = {
#include "player/amy_bird/bird_wing_tip_minus_z.inc"
};

static NJS_MATERIAL bird_wing_minus_z_materials[1] = {
#include "player/amy_bird/bird_wing_minus_z_materials.inc"
};

static Sint16 bird_wing_minus_z_meshes0[5] = {
#include "player/amy_bird/bird_wing_minus_z_meshes0.inc"
};

static NJS_COLOR bird_wing_minus_z_uvs0[4] = {
#include "player/amy_bird/bird_wing_minus_z_uvs0.inc"
};

static NJS_MESHSET bird_wing_minus_z_meshsets[1] = {
#include "player/amy_bird/bird_wing_minus_z_meshsets.inc"
};

static NJS_POINT3 bird_wing_minus_z_points[4] = {
#include "player/amy_bird/bird_wing_minus_z_points.inc"
};

static NJS_VECTOR bird_wing_minus_z_normals[4] = {
#include "player/amy_bird/bird_wing_minus_z_normals.inc"
};

static NJS_MODEL bird_wing_minus_z_model = {
#include "player/amy_bird/bird_wing_minus_z_model.inc"
};

static NJS_OBJECT bird_wing_minus_z = {
#include "player/amy_bird/bird_wing_minus_z.inc"
};

static NJS_MATERIAL bird_wing_tip_plus_z_materials[1] = {
#include "player/amy_bird/bird_wing_tip_plus_z_materials.inc"
};

static Sint16 bird_wing_tip_plus_z_meshes0[11] = {
#include "player/amy_bird/bird_wing_tip_plus_z_meshes0.inc"
};

static NJS_COLOR bird_wing_tip_plus_z_uvs0[9] = {
#include "player/amy_bird/bird_wing_tip_plus_z_uvs0.inc"
};

static NJS_MESHSET bird_wing_tip_plus_z_meshsets[1] = {
#include "player/amy_bird/bird_wing_tip_plus_z_meshsets.inc"
};

static NJS_POINT3 bird_wing_tip_plus_z_points[7] = {
#include "player/amy_bird/bird_wing_tip_plus_z_points.inc"
};

static NJS_VECTOR bird_wing_tip_plus_z_normals[7] = {
#include "player/amy_bird/bird_wing_tip_plus_z_normals.inc"
};

static NJS_MODEL bird_wing_tip_plus_z_model = {
#include "player/amy_bird/bird_wing_tip_plus_z_model.inc"
};

static NJS_OBJECT bird_wing_tip_plus_z = {
#include "player/amy_bird/bird_wing_tip_plus_z.inc"
};

static NJS_MATERIAL bird_wing_plus_z_materials[1] = {
#include "player/amy_bird/bird_wing_plus_z_materials.inc"
};

static Sint16 bird_wing_plus_z_meshes0[5] = {
#include "player/amy_bird/bird_wing_plus_z_meshes0.inc"
};

static NJS_COLOR bird_wing_plus_z_uvs0[4] = {
#include "player/amy_bird/bird_wing_plus_z_uvs0.inc"
};

static NJS_MESHSET bird_wing_plus_z_meshsets[1] = {
#include "player/amy_bird/bird_wing_plus_z_meshsets.inc"
};

static NJS_POINT3 bird_wing_plus_z_points[4] = {
#include "player/amy_bird/bird_wing_plus_z_points.inc"
};

static NJS_VECTOR bird_wing_plus_z_normals[4] = {
#include "player/amy_bird/bird_wing_plus_z_normals.inc"
};

static NJS_MODEL bird_wing_plus_z_model = {
#include "player/amy_bird/bird_wing_plus_z_model.inc"
};

static NJS_OBJECT bird_wing_plus_z = {
#include "player/amy_bird/bird_wing_plus_z.inc"
};

static NJS_MATERIAL bird_tail_materials[1] = {
#include "player/amy_bird/bird_tail_materials.inc"
};

static Sint16 bird_tail_meshes0[18] = {
#include "player/amy_bird/bird_tail_meshes0.inc"
};

static NJS_COLOR bird_tail_uvs0[15] = {
#include "player/amy_bird/bird_tail_uvs0.inc"
};

static NJS_MESHSET bird_tail_meshsets[1] = {
#include "player/amy_bird/bird_tail_meshsets.inc"
};

static NJS_POINT3 bird_tail_points[7] = {
#include "player/amy_bird/bird_tail_points.inc"
};

static NJS_VECTOR bird_tail_normals[7] = {
#include "player/amy_bird/bird_tail_normals.inc"
};

static NJS_MODEL bird_tail_model = {
#include "player/amy_bird/bird_tail_model.inc"
};

static NJS_OBJECT bird_tail = {
#include "player/amy_bird/bird_tail.inc"
};

static NJS_MATERIAL bird_head_materials[5] = {
#include "player/amy_bird/bird_head_materials.inc"
};

static Sint16 bird_head_meshes0[236] = {
#include "player/amy_bird/bird_head_meshes0.inc"
};

static Sint16 bird_head_meshes1[22] = {
#include "player/amy_bird/bird_head_meshes1.inc"
};

static Sint16 bird_head_meshes2[21] = {
#include "player/amy_bird/bird_head_meshes2.inc"
};

static Sint16 bird_head_meshes3[21] = {
#include "player/amy_bird/bird_head_meshes3.inc"
};

static Sint16 bird_head_meshes4[18] = {
#include "player/amy_bird/bird_head_meshes4.inc"
};

static NJS_COLOR bird_head_uvs0[207] = {
#include "player/amy_bird/bird_head_uvs0.inc"
};

static NJS_COLOR bird_head_uvs1[20] = {
#include "player/amy_bird/bird_head_uvs1.inc"
};

static NJS_COLOR bird_head_uvs2[16] = {
#include "player/amy_bird/bird_head_uvs2.inc"
};

static NJS_COLOR bird_head_uvs3[18] = {
#include "player/amy_bird/bird_head_uvs3.inc"
};

static NJS_COLOR bird_head_uvs4[16] = {
#include "player/amy_bird/bird_head_uvs4.inc"
};

static NJS_MESHSET bird_head_meshsets[5] = {
#include "player/amy_bird/bird_head_meshsets.inc"
};

static NJS_POINT3 bird_head_points[143] = {
#include "player/amy_bird/bird_head_points.inc"
};

static NJS_VECTOR bird_head_normals[143] = {
#include "player/amy_bird/bird_head_normals.inc"
};

static NJS_MODEL bird_head_model = {
#include "player/amy_bird/bird_head_model.inc"
};

static NJS_OBJECT bird_head = {
#include "player/amy_bird/bird_head.inc"
};

static NJS_MATERIAL bird_foot_minus_z_materials[1] = {
#include "player/amy_bird/bird_foot_minus_z_materials.inc"
};

static Sint16 bird_foot_minus_z_meshes0[47] = {
#include "player/amy_bird/bird_foot_minus_z_meshes0.inc"
};

static NJS_COLOR bird_foot_minus_z_uvs0[38] = {
#include "player/amy_bird/bird_foot_minus_z_uvs0.inc"
};

static NJS_MESHSET bird_foot_minus_z_meshsets[1] = {
#include "player/amy_bird/bird_foot_minus_z_meshsets.inc"
};

static NJS_POINT3 bird_foot_minus_z_points[12] = {
#include "player/amy_bird/bird_foot_minus_z_points.inc"
};

static NJS_VECTOR bird_foot_minus_z_normals[12] = {
#include "player/amy_bird/bird_foot_minus_z_normals.inc"
};

static NJS_MODEL bird_foot_minus_z_model = {
#include "player/amy_bird/bird_foot_minus_z_model.inc"
};

static NJS_OBJECT bird_foot_minus_z = {
#include "player/amy_bird/bird_foot_minus_z.inc"
};

static NJS_MATERIAL bird_foot_plus_z_materials[1] = {
#include "player/amy_bird/bird_foot_plus_z_materials.inc"
};

static Sint16 bird_foot_plus_z_meshes0[47] = {
#include "player/amy_bird/bird_foot_plus_z_meshes0.inc"
};

static NJS_COLOR bird_foot_plus_z_uvs0[38] = {
#include "player/amy_bird/bird_foot_plus_z_uvs0.inc"
};

static NJS_MESHSET bird_foot_plus_z_meshsets[1] = {
#include "player/amy_bird/bird_foot_plus_z_meshsets.inc"
};

static NJS_POINT3 bird_foot_plus_z_points[12] = {
#include "player/amy_bird/bird_foot_plus_z_points.inc"
};

static NJS_VECTOR bird_foot_plus_z_normals[12] = {
#include "player/amy_bird/bird_foot_plus_z_normals.inc"
};

static NJS_MODEL bird_foot_plus_z_model = {
#include "player/amy_bird/bird_foot_plus_z_model.inc"
};

static NJS_OBJECT bird_foot_plus_z = {
#include "player/amy_bird/bird_foot_plus_z.inc"
};

static NJS_MATERIAL bird_body_materials[5] = {
#include "player/amy_bird/bird_body_materials.inc"
};

static Sint16 bird_body_meshes0[54] = {
#include "player/amy_bird/bird_body_meshes0.inc"
};

static Sint16 bird_body_meshes1[21] = {
#include "player/amy_bird/bird_body_meshes1.inc"
};

static Sint16 bird_body_meshes2[35] = {
#include "player/amy_bird/bird_body_meshes2.inc"
};

static Sint16 bird_body_meshes3[23] = {
#include "player/amy_bird/bird_body_meshes3.inc"
};

static Sint16 bird_body_meshes4[20] = {
#include "player/amy_bird/bird_body_meshes4.inc"
};

static NJS_COLOR bird_body_uvs0[48] = {
#include "player/amy_bird/bird_body_uvs0.inc"
};

static NJS_COLOR bird_body_uvs1[18] = {
#include "player/amy_bird/bird_body_uvs1.inc"
};

static NJS_COLOR bird_body_uvs2[30] = {
#include "player/amy_bird/bird_body_uvs2.inc"
};

static NJS_COLOR bird_body_uvs3[20] = {
#include "player/amy_bird/bird_body_uvs3.inc"
};

static NJS_COLOR bird_body_uvs4[16] = {
#include "player/amy_bird/bird_body_uvs4.inc"
};

static NJS_MESHSET bird_body_meshsets[5] = {
#include "player/amy_bird/bird_body_meshsets.inc"
};

static NJS_POINT3 bird_body_points[62] = {
#include "player/amy_bird/bird_body_points.inc"
};

static NJS_VECTOR bird_body_normals[62] = {
#include "player/amy_bird/bird_body_normals.inc"
};

static NJS_MODEL bird_body_model = {
#include "player/amy_bird/bird_body_model.inc"
};

static NJS_OBJECT bird_body = {
#include "player/amy_bird/bird_body.inc"
};

/* the bird's root, which has no model. its child is the body, which wears
 * the pendant. the body's children are the feet. the body's siblings are the
 * head, the tail and the two wings. each wing's child is its tip */
static NJS_OBJECT bird_object = {
#include "player/amy_bird/bird_object.inc"
};

/* the flap's keys. bird_flap_mdata has a row for each node, numbered depth
 * first from bird_object at 0. each array here takes its node's number */
static NJS_MKEY_F bird_flap_pos0[15] = {
#include "player/amy_bird/bird_flap_pos0.inc"
};

static NJS_MKEY_A bird_flap_ang2[16] = {
#include "player/amy_bird/bird_flap_ang2.inc"
};

static NJS_MKEY_A bird_flap_ang3[16] = {
#include "player/amy_bird/bird_flap_ang3.inc"
};

static NJS_MKEY_A bird_flap_ang4[15] = {
#include "player/amy_bird/bird_flap_ang4.inc"
};

static NJS_MKEY_A bird_flap_ang5[16] = {
#include "player/amy_bird/bird_flap_ang5.inc"
};

static NJS_MKEY_A bird_flap_ang6[16] = {
#include "player/amy_bird/bird_flap_ang6.inc"
};

static NJS_MKEY_A bird_flap_ang7[16] = {
#include "player/amy_bird/bird_flap_ang7.inc"
};

static NJS_MKEY_A bird_flap_ang8[16] = {
#include "player/amy_bird/bird_flap_ang8.inc"
};

static NJS_MKEY_A bird_flap_ang9[16] = {
#include "player/amy_bird/bird_flap_ang9.inc"
};

static NJS_MDATA2 bird_flap_mdata[10] = {
#include "player/amy_bird/bird_flap_mdata.inc"
};

static NJS_MOTION bird_flap_motion = {
#include "player/amy_bird/bird_flap_motion.inc"
};

/* @hack type: an array of one, whose address bird_step loads to read its
 * motion at @(4,R3) (0x8c0e8a6e, 0x8c0e8a7c); as a plain object, the pool
 * holds the motion member's address, and 3 of bird_step's 340 words
 * differ */
static NJS_ACTION bird_flap_action[1] = {
#include "player/amy_bird/bird_flap_action.inc"
};

static void bird_draw(struct task *task)
{
    struct task_work *work;

    if (frame_step != 0) {
        return;
    }
    work = task->work;
    njSetTexture(&amy_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_action(bird_flap_action, VARS(work)->frame);
    njPopMatrix(1);
}

/* move the bird and draw it, unless an event has control. while player 0
 * takes a drum, the bird eases toward its perch over her. while she carries
 * the drum, it keeps to her position, and its step doesn't draw it.
 * otherwise it circles her, bobbing and facing her way. it flaps half a
 * frame a step, and half a frame more for each unit it moves */
static void bird_step(struct task *task)
{
    struct task_work *player = player_work[0];
    struct task_work *work = task->work;
    Float dx;
    Float dz;
    Float moved;
    NJS_POINT3 last;
    NJS_POINT3 to;
    NJS_VECTOR up;

    /* @bug memory: with no task for player 0, the step kills the bird's task
     * but goes on. unless an event has control, it then reads player->ang.y
     * through the null pointer (MOV.L @(24,R13),R5 at 0x8c0e898a) */
    if (player == NULL) {
        task_kill(task);
    }
    if (event_in_control() == TRUE) {
        return;
    }
    last = work->pos;
    if (amy_get_action(0) == AMY_TAKING_DRUM) {
        /* @hack spelling: up.x and up.z in one chained store, which loads 0.0F
         * into FR3 for both (FLDI0 FR3 at 0x8c0e87ee); as two stores, SHC
         * keeps 0.0F in FR15 and saves it, the stack frame grows from 72
         * bytes to 76, and 178 of 340 words differ */
        up.x = up.z = 0.0F;
        up.y = PERCH_HEIGHT;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateZ, player->ang.z);
        TURN(njRotateX, player->ang.x);
        TURN(njRotateY, -player->ang.y);
        njCalcVector(NULL, &up, &to);
        njPopMatrix(1);
        to.x += player->pos.x;
        to.y += player->pos.y;
        to.z += player->pos.z;
        dx = to.x - work->pos.x;
        dz = to.z - work->pos.z;
        work->ang.y = angle_approach(
            work->ang.y, (Angle)(atan2f(dz, -dx) * ANGLE_PER_RADIAN), 0x400);
        work->pos.x += (to.x - work->pos.x) * 0.1F;
        work->pos.y += (to.y - work->pos.y) * 0.1F;
        work->pos.z += (to.z - work->pos.z) * 0.1F;
    } else {
        if (amy_carries_drum(0) == TRUE) {
            work->pos = player->pos;
            return;
        }
        work->ang.y = angle_approach(work->ang.y,
                                     NJM_DEG_ANG(180) - player->ang.y, 0x400);
        work->ang.x += 16;
        to.x = player->pos.x + (njCos(work->ang.x) * BIRD_RADIUS);
        to.z = player->pos.z + (njSin(work->ang.x) * BIRD_RADIUS);
        to.y = player->pos.y + njSin(work->ang.z += 0x400) + BIRD_HEIGHT;
        if (njDistanceP2P(&work->pos, &to) < BIRD_JUMP_DISTANCE) {
            work->pos.x += (to.x - work->pos.x) * 0.25F;
            work->pos.y += (to.y - work->pos.y) * 0.25F;
            work->pos.z += (to.z - work->pos.z) * 0.25F;
        } else {
            work->pos = to;
        }
    }
    moved = njDistanceP2P(&last, &work->pos);
    /* @hack spelling: the frame tested where it's stored, which tests the sum
     * still in FR2 (FCMP/GT FR2,FR3 at 0x8c0e8a8e); stored in a statement of
     * its own, SHC loads it again for the test, and 26 of 340 words differ */
    if ((VARS(work)->frame += (moved * 0.5F) + 0.5F) >=
        (Float)bird_flap_action[0].motion->nbFrame) {
        VARS(work)->frame = 0.0F;
    }
    bird_draw(task);
    task_step_children(task);
}

/* start the bird BIRD_HEIGHT over player 0, facing her way. in any act of
 * STAGE_TWINKLE, a child of its task also draws it in the mirrors */
static void bird_init(struct task *task)
{
    struct task_work *player = player_work[0];
    struct task_work *work = task->work;

    /* @bug memory: with no task for player 0, bird_init kills the bird's task
     * but goes on, and reads player->pos through the null pointer
     * (MOV.L @(32,R13),R3 at 0x8c0e8b1a) */
    if (player == NULL) {
        task_kill(task);
    }
    work->pos = player->pos;
    work->pos.y += BIRD_HEIGHT;
    work->ang.y = NJM_DEG_ANG(180) - player->ang.y;
    task->draw = bird_draw;
    if (IN_STAGE(STAGE_TWINKLE)) {
        twinkle_reflect(task);
    }
    task->step = bird_step;
}

struct task *amy_bird_create(void)
{
    Uint16 stage_act;
    struct task *task;

    if (get_character() != CHARACTER_AMY) {
        return NULL;
    }
    if (game_mode != GAME_MODE_ACTION && game_mode != GAME_MODE_FIELD &&
        game_mode != GAME_MODE_TRIAL) {
        return NULL;
    }
    stage_act = stage_get_stage_act();
    if (stage_act < STAGE_ACT(STAGE_CHAOS0, 0)) {
        Uint16 stages[] = {
            STAGE_ACT(STAGE_TWINKLE, 1), STAGE_ACT(STAGE_TWINKLE, 2),
            STAGE_ACT(STAGE_FINAL, 0),   STAGE_ACT(STAGE_SHELTER, 0),
            STAGE_ACT(STAGE_SHELTER, 1), BIRD_STAGES_END,
        };
        Uint16 *stage;

        for (stage = stages; *stage != stage_act; stage++) {
            if (*stage == BIRD_STAGES_END) {
                return NULL;
            }
        }
    } else if (stage_act >= STAGE_ACT(STAGE_STATION_SQUARE, 0) &&
               stage_act < STAGE_ACT(STAGE_CART, 0)) {
        /* a field, where no bird flies while an event has control */
        if (event_in_control()) {
            return NULL;
        }
    } else {
        return NULL;
    }
    /* @hack redundant: the task tested against NULL, though both arms return
     * it. the test reads R0 at 0x8c0e8c4a; with task_create's result
     * returned directly, the call moves to the end and 26 of 82 words
     * differ */
    /* @hack spelling: the task assigned in the test, which copies it to R4 in
     * the BF/S's slot (MOV R0,R4 at 0x8c0e8c4e); assigned in a statement of
     * its own, SHC copies it first and tests R4, and 3 of 82 words differ */
    if ((task = task_create(TASK_WORK, 3, bird_init)) == NULL) {
        return NULL;
    }
    return task;
}

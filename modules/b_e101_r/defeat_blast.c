/* a blast for the boss's defeat: a ball, and a column with pieces
 * stacked up it. each is drawn twice, the second time mirrored in y about
 * the center. for a while, beams swing out from the center, and puffs burst
 * about it. nothing makes one: no image holds
 * e101_mk2_defeat_blast_create's address, and no branch in B_E101_R
 * reaches it */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_e101_r/camera.h"
#include "b_e101_r/defeat_blast.h"
#include "b_e101_r/files.h"
#include "b_egm2/blast.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "effect/beam.h"
#include "effect/bomb.h"
#include "system/task.h"
#include "system/util.h"

/* byte offsets of the ball's and the column's objects in E101R.NB. the boss's
 * other blasts draw the same ball */
#define BALL_OBJECT 0xaacc
#define COLUMN_OBJECT 0xdefc

/* the pieces stacked up the column */
#define PIECES 9

struct piece {
    Float height; /* up the column, before the ball's scale */
    Angle ang; /* the piece's turn about the column */
};

/* a defeat blast's record, in task->extra. its fields from steps to alpha
 * lie as the Egg Walker's blast record's do. that record keeps params
 * next, where this one keeps the stretches and the pieces first */
struct defeat_blast {
    Sint32 steps; /* the steps the blast has run, this one included */
    NJS_POINT3 pos; /* never written: the blast keeps params->pos */
    Angle beam_turn; /* beam_phase's turn each step */
    Angle beam_phase;
    Float beam_reach;
    Float size; /* the ball's size, which stays 0 */
    Float growth; /* never written or read */
    Float alpha;
    Float piece_stretch; /* the pieces' scale in y */
    Float column_stretch; /* the column's scale in y */
    struct piece pieces[PIECES];
    struct blast_params params; /* a copy, whose pos is the caller's point */
};

/* the scale of the ball's model for each unit of its size */
#define BALL_SCALE (1.0F / 63.0F)

/* while the beams swing, a puff bursts about the center every PUFF_EVERY
 * steps. its size is the sine of beam_phase plus PUFF_SIZE. it lies at
 * random in a cube about the center. the cube's side is PUFF_SPREAD times
 * the puff's size plus PUFF_NEAR. the puff bursts at PUFF_SCALE times its
 * size */
#define PUFF_EVERY 2
#define PUFF_SIZE 0.1F
#define PUFF_SPREAD 35.0F
#define PUFF_NEAR 5.0F
#define PUFF_SCALE 0.6F

/* each piece's object, as a byte offset in E101R.NB, and its height up the
 * column */
static struct {
    Sint32 object;
    Float height;
} defeat_pieces[PIECES] = {
    {0xd9c4, 66.5411F}, {0xd48c, 64.2111F}, {0xcf54, 60.7367F},
    {0xca1c, 55.9158F}, {0xc4e4, 50.213F},  {0xbfac, 42.8188F},
    {0xba74, 33.6562F}, {0xb53c, 22.6059F}, {0xb004, 7.635F},
};
/* the beams' params. beam_create copies them, so defeat_blast_beams sets
 * length and max_speed in place before each call */
static struct beam_params defeat_beam_params = {
    20, {0.3F, 2.0F}, {0xffffffff, 0x00ffffff}, 0x5b, 0.0F, 1.3F,
};

/* a scattered puff's offset from its center, before
 * e101_mk2_defeat_blast_scatter turns it and scales it to the radius */
static const NJS_POINT3 defeat_scatter_dir = {0.0F, 0.0F, 1.0F};

static void defeat_blast_beams(struct task *task)
{
    struct defeat_blast *blast = task->extra;
    struct blast_params *params = &blast->params;

    blast->beam_phase += blast->beam_turn;
    blast->beam_reach = njSin(blast->beam_phase) * params->beam_length;
    if (blast->beam_reach > 0.0F) {
        defeat_beam_params.length = blast->beam_reach;
        defeat_beam_params.max_speed = params->beam_speed;
        beam_create(params->pos, &defeat_beam_params);
    }
}

static void defeat_blast_puff(struct task *task)
{
    struct defeat_blast *blast = task->extra;
    struct blast_params *params = &blast->params;
    Float size = njSin(blast->beam_phase) + PUFF_SIZE;
    Float spread = (size * PUFF_SPREAD) + PUFF_NEAR;
    NJS_POINT3 pos;

    pos.x = params->pos->x + (rand() * spread * RAND_SCALE) - (spread * 0.5F);
    pos.y = params->pos->y + (rand() * spread * RAND_SCALE) - (spread * 0.5F);
    pos.z = params->pos->z + (rand() * spread * RAND_SCALE) - (spread * 0.5F);
    bomb_create(&pos, size * PUFF_SCALE);
}

void e101_mk2_defeat_blast_scatter(NJS_POINT3 *pos, Float radius, Float scale)
{
    NJS_POINT3 offset;

    njPushMatrix(&_unit_matrix_);
    /* TURN reads its angle once to test it against 0, and again to turn by
     * it. each turn so calls rand() once for the test. unless that gives 0,
     * it calls rand() again for the angle it turns by */
    TURN(njRotateX, (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE));
    TURN(njRotateY, (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE));
    njScale(NULL, radius, radius, radius);
    njCalcPoint(NULL, (NJS_POINT3 *)&defeat_scatter_dir, &offset);
    njPopMatrix(1);
    VECTOR_SUM(&offset, &offset, pos);
    bomb_create(&offset, scale);
}

/* while the beams swing, make a beam each step and a puff every PUFF_EVERY
 * steps */
/* @hack redundant: register keeps task in R14 and the blast in R13, as MOV
 * R4,R14 at 0x0c905842 sets it; without it, the two trade registers, and 8
 * of 31 words differ */
static void defeat_blast_spray(register struct task *task)
{
    struct defeat_blast *blast = task->extra;
    struct blast_params *params = &blast->params;

    if (blast->steps > params->beam_start && blast->steps < params->beam_end) {
        defeat_blast_beams(task);
        if (blast->steps % PUFF_EVERY == 0) {
            defeat_blast_puff(task);
        }
    }
}

/* draw the ball, the column and the pieces unlit, in white at alpha, with
 * additive blend factors. all three scale with the ball's size, and draw
 * again mirrored in y about the center. the mirrored pieces turn the other
 * way */
static void defeat_blast_draw(struct task *task)
{
    struct defeat_blast *blast = task->extra;
    struct blast_params *params = &blast->params;
    Float scale;
    int i;

    njSetTexture(e101_r_texlist);
    scale = blast->size * BALL_SCALE;
    njPushMatrix(NULL);
    draw_control_save();
    draw_attr_save();
    njControl3D(NJD_CONTROL_3D_CONSTANT_ATTR |
                NJD_CONTROL_3D_CONSTANT_MATERIAL);
    draw_set_material(blast->alpha, 1.0F, 1.0F, 1.0F);
    njSetConstantAttr(~(NJD_SA_MASK | NJD_DA_MASK),
                      NJD_SA_SRC | NJD_DA_ONE | NJD_FLAG_IGNORE_LIGHT);
    njTranslateV(NULL, params->pos);
    njPushMatrix(NULL);
    njScale(NULL, scale, scale, scale);
    draw_tree((NJS_OBJECT *)(e101_r_models + BALL_OBJECT));
    njScale(NULL, 1.0F, -1.0F, 1.0F);
    draw_tree((NJS_OBJECT *)(e101_r_models + BALL_OBJECT));
    njPopMatrix(1);
    njScale(NULL, scale, scale, scale);
    njPushMatrix(NULL);
    njScale(NULL, 1.0F, blast->column_stretch, 1.0F);
    draw_tree((NJS_OBJECT *)(e101_r_models + COLUMN_OBJECT));
    njPopMatrix(1);
    for (i = 0; i < PIECES; i++) {
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, blast->pieces[i].height, 0.0F);
        TURN(njRotateY, blast->pieces[i].ang);
        njScale(NULL, 1.0F, blast->piece_stretch, 1.0F);
        draw_tree((NJS_OBJECT *)(e101_r_models + defeat_pieces[i].object));
        njPopMatrix(1);
    }
    njScale(NULL, 1.0F, -1.0F, 1.0F);
    njPushMatrix(NULL);
    njScale(NULL, 1.0F, blast->column_stretch, 1.0F);
    draw_tree((NJS_OBJECT *)(e101_r_models + COLUMN_OBJECT));
    njPopMatrix(1);
    for (i = 0; i < PIECES; i++) {
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, blast->pieces[i].height, 0.0F);
        TURN(njRotateY, -blast->pieces[i].ang);
        njScale(NULL, 1.0F, blast->piece_stretch, 1.0F);
        draw_tree((NJS_OBJECT *)(e101_r_models + defeat_pieces[i].object));
        njPopMatrix(1);
    }
    draw_attr_restore();
    draw_control_restore();
    njPopMatrix(1);
}

/* @hack redundant: register keeps task in R14 and the blast in R13, as MOV
 * R4,R14 at 0x0c905c22 sets it; without it, the two trade registers, and 11
 * of 37 words differ */
static void defeat_blast_run(register struct task *task)
{
    struct defeat_blast *blast = task->extra;
    struct blast_params *params = &blast->params;

    ++blast->steps;
    if (blast->steps >= params->beam_end && blast->steps >= params->end) {
        e101_r_camera_set_mode(E101_R_CAMERA_ORBIT_END);
        task_kill(task);
    } else {
        defeat_blast_spray(task);
        e101_r_camera_set_blast_size(blast->size);
        defeat_blast_draw(task);
    }
}

void e101_mk2_defeat_blast_create(struct blast_params *params)
{
    struct task *task;
    struct defeat_blast *blast;
    int i;

    if ((task = task_create(TASK_WORK, 4, defeat_blast_run)) != NULL) {
        blast = heap_alloc(sizeof(struct defeat_blast));
        if (blast == NULL) {
            task_destroy(task);
            return;
        }
        task->extra = blast;
        blast->steps = 0;
        blast->beam_turn =
            NJM_DEG_ANG(180) / (params->beam_end - params->beam_start);
        blast->beam_phase = 0;
        blast->beam_reach = 0.0F;
        blast->size = 0.0F;
        blast->alpha = 0.8F;
        blast->params = *params;
        blast->piece_stretch = 1.0F;
        blast->column_stretch = 2.0F;
        for (i = 0; i < PIECES; i++) {
            blast->pieces[i].height = defeat_pieces[i].height;
            blast->pieces[i].ang =
                (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
        }
        task->draw = defeat_blast_draw;
    }
}

/* Zero's last explosion, which erobo_explosion_start makes as Zero is
 * defeated */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_egm2/blast.h"
#include "b_robo/banks.h"
#include "b_robo/camera.h"
#include "b_robo/effects.h"
#include "b_robo/explosion.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "effect/bomb.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

/* a random angle, from 0 up to a whole turn */
#define RANDOM_TURN()                                                          \
    ((Angle)(rand() * RAND_SCALE * (2 * NJD_PI) * ANGLE_PER_RADIAN))

/* the rings stacked up the column */
#define RINGS 9

/* the ball's shell and its column, as offsets into the model bank */
#define SHELL_OBJECT 0x12d08
#define COLUMN_OBJECT 0x16138

/* the crackle of the swinging beams, and the boom as the ball starts */
#define SE_CRACKLE 26
#define SE_BOOM 0x300

/* the shell's scale at a size of 1 */
#define BALL_SCALE (1.0F / 63.0F)
/* the ball's alpha until it fades */
#define BALL_ALPHA 0.8F

/* the part of its growth the ball keeps each step once grown. FADE_SWELL
 * multiplies its size each step as it fades */
#define GROWTH_DRAG 0.95F
#define FADE_SWELL 1.005F

/* while the beams swing, a puff comes every PUFF_EVERY steps. its size is
 * the swing's sine plus PUFF_SIZE, and it is drawn at PUFF_SCALE times
 * that. it lies at random in a cube round the explosion's middle. the
 * cube is PUFF_CUBE wide, and PUFF_SPREAD wider for each unit of the
 * size */
#define PUFF_EVERY 7
#define PUFF_SIZE 0.1F
#define PUFF_SPREAD 35.0F
#define PUFF_CUBE 5.0F
#define PUFF_SCALE 0.6F
/* from grow_start until fade_start, a puff at SHELL_PUFF_SCALE comes on
 * the shell every SHELL_PUFF_EVERY steps */
#define SHELL_PUFF_EVERY 3
#define SHELL_PUFF_SCALE 0.5F

/* a ring's turn a step. the even rings turn one way, and the odd rings the
 * other */
#define RING_TURN NJM_DEG_ANG(1)
/* the part of their height the rings and the column keep each step from
 * grow_start */
#define RING_FLATTEN 0.99F
#define COLUMN_SHRINK 0.97F

struct explosion_ring {
    Float height;
    Angle ang;
};

/* an explosion's record, at its task's extra */
struct explosion {
    Sint32 steps; /* the steps since the explosion's start */
    /* blast.c's blast keeps a copy of *params.pos here under
     * BLAST_COPY_POS. the explosion ignores the flag and leaves pos unused */
    NJS_POINT3 pos;
    /* the turn beam_phase takes each step: half a turn over the steps from
     * beam_start to beam_end */
    Angle beam_turn;
    Angle beam_phase;
    Float beam_reach;
    Float size;
    Float growth; /* the size the ball last gained in a step */
    Float alpha;
    /* how tall the rings and the column are drawn, as a scale in y */
    Float ring_scale;
    Float column_scale;
    struct explosion_ring rings[RINGS];
    struct blast_params params;
};

struct ring_model {
    Uint32 object; /* an offset into the model bank */
    /* how high up the column the ring sits, in the model's units */
    Float height;
};

/* a shell puff's way out from the ball's middle, before
 * explosion_shell_puff turns it at random */
static const NJS_POINT3 shell_dir = {0.0F, 0.0F, 1.0F};

static struct ring_model ring_models[RINGS] = {
    {0x15c00, 66.5411F}, {0x156c8, 64.2111F}, {0x15190, 60.7367F},
    {0x14c58, 55.9158F}, {0x14720, 50.213F},  {0x141e8, 42.8188F},
    {0x13cb0, 33.6562F}, {0x13778, 22.6059F}, {0x13240, 7.635F},
};

/* make a puff near the explosion's middle, as the PUFF_ constants say */
static void explosion_puff(struct task *task)
{
    struct explosion *explosion = task->extra;
    struct blast_params *params = &explosion->params;
    NJS_POINT3 pos;
    Float size = njSin(explosion->beam_phase) + PUFF_SIZE;
    Float spread = (size * PUFF_SPREAD) + PUFF_CUBE;

    pos.x = params->pos->x + (rand() * spread * RAND_SCALE) - (spread * 0.5F);
    pos.y = params->pos->y + (rand() * spread * RAND_SCALE) - (spread * 0.5F);
    pos.z = params->pos->z + (rand() * spread * RAND_SCALE) - (spread * 0.5F);
    bomb_create(&pos, size * PUFF_SCALE);
}

/* make a puff at scale at a random point of the sphere of radius about
 * center */
static void explosion_shell_puff(NJS_POINT3 *center, Float radius, Float scale)
{
    NJS_POINT3 point;

    njPushMatrix(&_unit_matrix_);
    /* TURN reads its angle twice, so each turn draws two random angles.
     * the first only decides whether the matrix turns at all */
    TURN(njRotateX, RANDOM_TURN());
    TURN(njRotateY, RANDOM_TURN());
    njScale(NULL, radius, radius, radius);
    njCalcPoint(NULL, (NJS_POINT3 *)&shell_dir, &point);
    njPopMatrix(1);
    VECTOR_SUM(&point, &point, center);
    bomb_create(&point, scale);
}

/* while steps lies strictly between beam_start and beam_end, move the
 * swing on a step. make a beam as long as the swing reaches, and every
 * PUFF_EVERY steps a puff. about half the puffs crackle, at random */
static void explosion_beams(struct task *arg)
{
    /* @hack spelling: the task through a local, which SHC keeps in R12
     * (MOV R4,R12 at 0x0c903ce4); as the parameter, 68 of 73 words
     * differ */
    struct task *task = arg;
    struct explosion *explosion = task->extra;
    struct blast_params *params = &explosion->params;

    if (explosion->steps > params->beam_start &&
        explosion->steps < params->beam_end) {
        explosion->beam_phase += explosion->beam_turn;
        explosion->beam_reach =
            njSin(explosion->beam_phase) * params->beam_length;
        erobo_beams_create(params->pos, explosion->beam_reach,
                           params->beam_speed);
        if (explosion->steps % PUFF_EVERY == 0) {
            explosion_puff(task);
            if (rand() * RAND_SCALE <= 0.5F) {
                sound_play_at(SE_CRACKLE, 0, 0, -16, params->pos->x,
                              params->pos->y, params->pos->z);
            }
        }
    }
}

/* the ball starts with a boom at grow_start. it grows evenly to size by
 * grow_end, then on more slowly each step. past fade_start, it swells and
 * fades out by end. from grow_start, the rings flatten and turn, and the
 * column shrinks */
static void explosion_grow(struct task *task)
{
    struct explosion *explosion = task->extra;
    struct blast_params *params = &explosion->params;
    Float was;
    Float fade;
    int i;

    if (explosion->steps < params->grow_start) {
        return;
    }
    if (explosion->steps == params->grow_start) {
        sound_play(SE_BOOM, 0, 0, 0);
    }
    if (explosion->steps < params->grow_end) {
        was = explosion->size;
        explosion->size +=
            params->size / (params->grow_end - params->grow_start);
        explosion->growth = explosion->size - was;
    } else {
        explosion->size += explosion->growth;
        explosion->growth *= GROWTH_DRAG;
    }
    if (explosion->steps > params->fade_start) {
        explosion->size *= FADE_SWELL;
    } else if (explosion->steps % SHELL_PUFF_EVERY == 0) {
        explosion_shell_puff(params->pos, explosion->size, SHELL_PUFF_SCALE);
    }
    explosion->ring_scale *= RING_FLATTEN;
    explosion->column_scale *= COLUMN_SHRINK;
    for (i = 0; i < RINGS; i++) {
        explosion->rings[i].ang += i % 2 == 0 ? RING_TURN : -RING_TURN;
    }
    if (explosion->steps > params->fade_start) {
        fade = 1.0F - ((Float)(explosion->steps - params->fade_start) /
                       (params->end - params->fade_start));
        explosion->alpha = fade * BALL_ALPHA;
    }
}

/* draw the shell, and its mirror image below it. then draw the column and
 * its rings, and their mirror image. it draws on every step, not only a
 * frame's first */
static void explosion_draw(struct task *task)
{
    struct explosion *explosion = task->extra;
    struct blast_params *params = &explosion->params;
    Float scale;
    int i;
    Angle ang;

    njSetTexture(erobo_textures);
    scale = explosion->size * BALL_SCALE;
    njPushMatrix(NULL);
    draw_control_save();
    draw_attr_save();
    njControl3D(NJD_CONTROL_3D_CONSTANT_ATTR |
                NJD_CONTROL_3D_CONSTANT_MATERIAL);
    draw_set_material(explosion->alpha, 1.0F, 1.0F, 1.0F);
    njSetConstantAttr(~(NJD_SA_MASK | NJD_DA_MASK),
                      NJD_SA_SRC | NJD_DA_ONE | NJD_FLAG_IGNORE_LIGHT);
    njTranslateV(NULL, params->pos);
    njPushMatrix(NULL);
    njScale(NULL, scale, scale, scale);
    draw_tree((NJS_OBJECT *)(erobo_models + SHELL_OBJECT));
    njScale(NULL, 1.0F, -1.0F, 1.0F);
    draw_tree((NJS_OBJECT *)(erobo_models + SHELL_OBJECT));
    njPopMatrix(1);
    njScale(NULL, scale, scale, scale);
    njPushMatrix(NULL);
    njScale(NULL, 1.0F, explosion->column_scale, 1.0F);
    draw_tree((NJS_OBJECT *)(erobo_models + COLUMN_OBJECT));
    njPopMatrix(1);
    for (i = 0; i < RINGS; i++) {
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, explosion->rings[i].height, 0.0F);
        TURN_VIA(njRotateY, explosion->rings[i].ang, ang);
        njScale(NULL, 1.0F, explosion->ring_scale, 1.0F);
        draw_tree((NJS_OBJECT *)(erobo_models + ring_models[i].object));
        njPopMatrix(1);
    }
    njScale(NULL, 1.0F, -1.0F, 1.0F);
    njPushMatrix(NULL);
    njScale(NULL, 1.0F, explosion->column_scale, 1.0F);
    draw_tree((NJS_OBJECT *)(erobo_models + COLUMN_OBJECT));
    njPopMatrix(1);
    for (i = 0; i < RINGS; i++) {
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, explosion->rings[i].height, 0.0F);
        TURN_VIA(njRotateY, -explosion->rings[i].ang, ang);
        njScale(NULL, 1.0F, explosion->ring_scale, 1.0F);
        draw_tree((NJS_OBJECT *)(erobo_models + ring_models[i].object));
        njPopMatrix(1);
    }
    draw_attr_restore();
    draw_control_restore();
    njPopMatrix(1);
}

/* run the explosion a step: its beams while they last, its ball, and the
 * camera, then draw it. the explosion goes once the beams and the ball are
 * both over */
static void explosion_run(struct task *task)
{
    struct explosion *explosion = task->extra;
    /* @hack spelling: params through task->extra again, which gives the task
     * R14 and the record R13 (MOV R4,R14 at 0x0c904382); through explosion,
     * the two trade, and 13 of 46 words differ */
    struct blast_params *params = &((struct explosion *)task->extra)->params;

    ++explosion->steps;
    if (explosion->steps >= params->beam_end &&
        explosion->steps >= params->end) {
        task_kill(task);
        return;
    }
    explosion_beams(task);
    explosion_grow(task);
    erobo_camera_set_reach(explosion->size);
    if (explosion->steps >= params->fade_start) {
        erobo_camera_set_mode(EROBO_CAMERA_CIRCLE_OUT);
    }
    explosion_draw(task);
}

void erobo_explosion_create(struct blast_params *params)
{
    struct task *task;
    struct explosion *explosion;
    int i;

    if ((task = task_create(TASK_WORK, 4, explosion_run)) == NULL) {
        return;
    }
    explosion = heap_alloc(sizeof(struct explosion));
    if (explosion == NULL) {
        task_destroy(task);
        return;
    }
    task->extra = explosion;
    explosion->steps = 0;
    explosion->beam_turn = 0x8000 / (params->beam_end - params->beam_start);
    explosion->beam_phase = 0;
    explosion->beam_reach = 0.0F;
    explosion->size = 0.0F;
    explosion->alpha = BALL_ALPHA;
    explosion->params = *params;
    explosion->ring_scale = 1.0F;
    explosion->column_scale = 2.0F;
    for (i = 0; i < RINGS; i++) {
        explosion->rings[i].height = ring_models[i].height;
        explosion->rings[i].ang = RANDOM_TURN();
    }
    task->draw = explosion_draw;
}

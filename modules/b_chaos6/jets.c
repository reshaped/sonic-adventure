/* Chaos 6's jets: forty jets of water round the boss. each gathers as a
 * splash, then shoots out to its reach along the way it points. every other
 * jet tilts up or down at player 0 as it fires, and each knocks him back
 * once at most. the glow on the boss's tail fades in as BOSS_JETS starts,
 * before the jets fire. it fades out as the mode ends, after they have
 * drawn back in */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos6/boss.h"
#include "b_chaos6/jets.h"
#include "b_chaos6/textures.h"
#include "chaos/steer.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/motion.h"
#include "draw/tree.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the jets' task fires JETS_AT_ONCE of its JETS jets every JETS_WAIT
 * steps. it kills itself on the JETS_GONE_TIME'th step, counting the one
 * that draws them back in */
#define JETS 40
#define JETS_AT_ONCE 10
#define JETS_WAIT 45
#define JETS_GONE_TIME 100

/* a gathering splash shows c6_ht01 to c6_ht16, chaos6_effect_texlist's
 * textures 0 to SPLASH_LAST, for SPLASH_STEPS steps each. it then holds the
 * last */
#define SPLASH_LAST 15
#define SPLASH_STEPS 6
/* a jet's reach, and the length it grows or shrinks by a step. JET_FADE is
 * how much its alpha drops a step once it has drawn back in */
#define JET_REACH 220.0F
#define JET_SPEED 5.5F
#define JET_FADE (1.0F / 60.0F)
/* the jet's model has its tip 7.621864 along x. JET_STRETCH, its inverse,
 * brings the tip to 1 */
#define JET_STRETCH 0.1312015F
#define SE_JET 391

/* how much the glow's alpha changes a step as it fades in or out */
#define GLOW_FADE (1.0F / 60.0F)

/* in a jet's work->flags */
#define JET_OUT 0x100 /* its model draws */
#define JET_AIM 0x200 /* it tilts toward player 0 as it fires */
#define JET_HIT 0x400 /* it has knocked player 0 back */

struct jet_vars {
    Float alpha;
    Sint8 steps; /* the steps its splash's texture has shown */
    Sint8 texture; /* its splash's texture in chaos6_effect_texlist */
};

#define JET_VARS(work) ((struct jet_vars *)&(work)->vars)

/* the jets' task's vars, which start 4 bytes into its work->vars */
struct jets_vars {
    /* the steps since the last jets fired, then since they drew back in */
    Sint16 steps;
    Sint16 next; /* the next jet to fire, by its index in jet_tasks */
};

/* @hack type: the vars cast from vars.w[2], which keeps their address,
 * work + 12, in R12 as the jets fire (0x0c903d9c); without it, cast from
 * vars past a pad of 4 bytes, 98 of 224 words differ */
#define JETS_VARS(work) ((struct jets_vars *)&(work)->vars.w[2])

struct glow_vars {
    Float alpha;
};

#define GLOW_VARS(work) ((struct glow_vars *)&(work)->vars)

/* a jet's modes. the jets' task sets JET_FIRE and JET_SHRINK, and the jet
 * takes the others itself */
enum jet_mode { JET_GATHER, JET_FIRE, JET_GROW, JET_HOLD, JET_SHRINK, JET_DIM };

enum jets_mode { JETS_START, JETS_FIRE, JETS_HOLD, JETS_END_ALL, JETS_GONE };

enum glow_mode { GLOW_IN, GLOW_HOLD, GLOW_OUT };

/* the glow's place and turn in the frame of chaos6_tail_matrix */
#pragma section HEAD
static Angle tail_glow_ang_x = 0x1400;
static Angle tail_glow_ang_y = 0x4000;
static Angle tail_glow_ang_z = 0;
static Float tail_glow_x = 12.0F;
static Float tail_glow_y = 0.0F;
static Float tail_glow_z = 0.0F;
#pragma section

static const Uint8 jet_shape_count = 1;

static NJS_MATERIAL jet_materials[1] = {
#include "b_chaos6/jets/jet_materials.inc"
};

static Sint16 jet_meshes[46] = {
#include "b_chaos6/jets/jet_meshes.inc"
};

static NJS_COLOR jet_uvs[39] = {
#include "b_chaos6/jets/jet_uvs.inc"
};

static NJS_MESHSET jet_meshsets[1] = {
#include "b_chaos6/jets/jet_meshsets.inc"
};

static NJS_POINT3 jet_points[16] = {
#include "b_chaos6/jets/jet_points.inc"
};

static NJS_VECTOR jet_normals[16] = {
#include "b_chaos6/jets/jet_normals.inc"
};

static NJS_MODEL jet_model = {
#include "b_chaos6/jets/jet_model.inc"
};

static NJS_OBJECT jet_object = {
#include "b_chaos6/jets/jet_object.inc"
};

static NJS_MATERIAL tail_glow_materials[1] = {
#include "b_chaos6/jets/tail_glow_materials.inc"
};

static Sint16 tail_glow_meshes[216] = {
#include "b_chaos6/jets/tail_glow_meshes.inc"
};

static NJS_COLOR tail_glow_uvs[192] = {
#include "b_chaos6/jets/tail_glow_uvs.inc"
};

static NJS_MESHSET tail_glow_meshsets[1] = {
#include "b_chaos6/jets/tail_glow_meshsets.inc"
};

static NJS_POINT3 tail_glow_points[81] = {
#include "b_chaos6/jets/tail_glow_points.inc"
};

static NJS_VECTOR tail_glow_normals[81] = {
#include "b_chaos6/jets/tail_glow_normals.inc"
};

static NJS_MODEL tail_glow_model = {
#include "b_chaos6/jets/tail_glow_model.inc"
};

static NJS_OBJECT tail_glow_object = {
#include "b_chaos6/jets/tail_glow_object.inc"
};

/* a cylinder of radius 3, turned a quarter about z to lie along the jet.
 * each step centers it halfway along the jet, with half the jet's length
 * as its size_y */
static struct collision_shape jet_shapes[1] = {
    {SHAPE_CYLINDER2 | SHAPE_TURN_YZX,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     3.0F,
     0.0F,
     0.0F,
     {0, 0, 0x4000},
     0,
     NULL},
};

/* the sprite of a jet's splash. each draw sets jet_splash_frame's texid to
 * the jet's texture */
static NJS_TEXANIM jet_splash_frame = {
#include "b_chaos6/jets/jet_splash_frame.inc"
};
static NJS_SPRITE jet_splash = {
#include "b_chaos6/jets/jet_splash.inc"
};

/* where the jets start round the boss: four pairs of a height and a reach,
 * which the jets take in turn */
static Float jet_places[8] = {47.0F, 20.0F, 37.0F, 25.0F,
                              27.0F, 26.6F, 17.0F, 25.4F};

/* the glow's constant material. each draw sets its alpha to the glow's */
static NJS_ARGB tail_glow_color = {1.0F, 1.0F, 1.0F, 1.0F};

/* TRUE from chaos6_tail_glow_start until chaos6_tail_glow_end */
static Sint8 tail_glow_on;
static struct task *jet_tasks[JETS]; /* each jet's, in the order they fire */
/* chaos6_jets_step clears each entry as it makes each jet, and nothing
 * reads it */
static Sint8 dat_0ca91c4c[JETS];

/* draw the splash at the jet's tip, scl.x out along the way it points, and
 * while JET_OUT is set the jet's model, stretched to scl.x. it draws only on
 * a frame's first step */
static void chaos6_jet_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float alpha;
    Angle ang;

    if (frame_step) {
        return;
    }
    njFogDisable();
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    alpha = JET_VARS(work)->alpha;
    draw_set_material(alpha, alpha, alpha, alpha);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    njTranslate(NULL, work->scl.x, 0.0F, 0.0F);
    /* @hack spelling: texid stored through jet_splash_frame's address, which
     * the pool holds (0x0c90389e), at an offset of 16 (0x0c9038a4); without
     * it, 13 of 138 words differ */
    (&jet_splash_frame)->texid = (Sint16)JET_VARS(work)->texture;
    njDrawSprite3D(&jet_splash, 0,
                   NJD_SPRITE_COLOR | NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
    njPopMatrix(1);
    njFogEnable();
    /* the model's bounding radius takes the jet's length, for clipping */
    /* @hack spelling: r stored through jet_model's address, which the pool
     * holds (0x0c9038b8), at an offset of 36 (0x0c9038c0); without it, 56 of
     * 138 words differ */
    (&jet_model)->r = work->scl.x;
    if (work->flags & JET_OUT) {
        njSetTexture(&chaos6_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        njScale(NULL, work->scl.x * JET_STRETCH, 1.0F, 1.0F);
        draw_tree(&jet_object);
        njPopMatrix(1);
    }
}

static void chaos6_jet_run(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_shape *shape = work->collision->shapes;
    NJS_VECTOR toward;

    switch (work->mode) {
    case JET_GATHER:
        if (++JET_VARS(work)->steps >= SPLASH_STEPS) {
            JET_VARS(work)->steps = 0;
            if (++JET_VARS(work)->texture >= SPLASH_LAST) {
                work->mode = JET_HOLD;
            }
        }
        break;
    case JET_FIRE:
        work->flags |= JET_OUT;
        if (work->flags & JET_AIM) {
            toward = player_work[0]->pos;
            njSubVector(&toward, &work->pos);
            work->ang.z =
                (Angle)(atan2f(toward.y, njSqrt((toward.x * toward.x) +
                                                (toward.z * toward.z))) *
                        ANGLE_PER_RADIAN);
        }
        sound_play(SE_JET, 0, 0, 0);
        work->mode = JET_GROW;
        break;
    case JET_GROW:
        work->scl.x += JET_SPEED;
        if (work->scl.x >= JET_REACH) {
            work->mode = JET_HOLD;
        }
        break;
    case JET_HOLD:
        break;
    case JET_SHRINK:
        if ((work->scl.x -= JET_SPEED) <= 0.0F) {
            work->flags &= ~JET_OUT;
            work->mode = JET_DIM;
        }
        break;
    case JET_DIM:
        JET_VARS(work)->alpha -= JET_FADE;
        if (JET_VARS(work)->alpha <= 0.0F) {
            task_kill(task);
            return;
        }
        break;
    }
    shape->center.x = work->scl.x * 0.5F;
    shape->size_y = work->scl.x * 0.5F;
    /* @hack spelling: the knock back called inside the test, after a comma,
     * which loads player_set_mode's address into R1 for the JSR at
     * 0x0c903b98; without it, as a statement of its own, the call goes
     * through R3, and 2 of 209 words differ */
    if (!(work->flags & JET_HIT) && (work->collision->flags & COLLISION_HIT) &&
        work->collision->hit->kind == COLLISION_PLAYER &&
        (player_set_mode(0, PLAYER_MODE_KNOCK_BACK), TRUE)) {
        work->flags |= JET_HIT;
        task->parent->work->flags |= JETS_HIT;
    }
    chaos6_jet_draw(task);
    collision_register(work);
}

static void chaos6_jet_start(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, jet_shapes, COLLISION_OBJECT, 1, jet_shape_count);
    task->work->collision->radius = 200.0F;
    task->step = chaos6_jet_run;
    task->draw = chaos6_jet_draw;
    work->scl.x = 0.0F;
    JET_VARS(work)->alpha = 1.0F;
}

/* the first step places each jet at the next pair of jet_places, turned a
 * random way about the boss and lifted by his model's height. each step
 * then runs the jets' steps */
void chaos6_jets_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *jet;
    struct task *made;
    int i;
    int place;
    NJS_POINT3 out;
    Angle ang;

    switch (work->mode) {
    case JETS_START:
        for (i = place = 0; i < JETS; i++) {
            made = task_create_child(TASK_WORK, chaos6_jet_start, task);
            jet_tasks[i] = made;
            jet = made->work;
            njPushMatrix(&_unit_matrix_);
            jet->ang.y = (Angle)RANDOM_ANGLE();
            TURN_VIA(njRotateY, jet->ang.y, ang);
            out.x = jet_places[place + 1];
            out.y = jet_places[place];
            out.z = 0.0F;
            njCalcPoint(NULL, &out, &jet->pos);
            njPopMatrix(1);
            njAddVector(&jet->pos, &boss_work->pos);
            jet->pos.y += chaos_params->model_y;
            place += 2;
            if (place >= 8) {
                place = 0;
            }
            dat_0ca91c4c[i] = 0;
        }
        work->mode = JETS_FIRE;
        break;
    case JETS_FIRE:
        if (++JETS_VARS(work)->steps >= JETS_WAIT) {
            for (i = JETS_AT_ONCE; i != 0; i--) {
                jet = jet_tasks[JETS_VARS(work)->next]->work;
                JETS_VARS(work)->steps = 0;
                jet->mode = JET_FIRE;
                if (!(JETS_VARS(work)->next & 1)) {
                    jet->flags |= JET_AIM;
                }
                JETS_VARS(work)->next++;
            }
            if (JETS_VARS(work)->next >= JETS) {
                work->mode = JETS_HOLD;
            }
        }
        /* fall through */
    case JETS_HOLD:
        if (work->flags & JETS_END) {
            work->mode = JETS_END_ALL;
        }
        break;
    case JETS_END_ALL:
        for (i = 0; i < JETS; i++) {
            jet_tasks[i]->work->mode = JET_SHRINK;
        }
        JETS_VARS(work)->steps = 0;
        work->mode = JETS_GONE;
        /* fall through */
    case JETS_GONE:
        if (++JETS_VARS(work)->steps >= JETS_GONE_TIME) {
            task_kill(task);
            return;
        }
        break;
    }
    task_step_children(task);
}

/* draw tail_glow_object, textured with c6_tail, at its place in
 * chaos6_tail_matrix's frame, in the glow's alpha */
static void chaos6_tail_glow_draw(struct task *task)
{
    struct task_work *work = task->work;

    njSetTexture(&chaos6_effect_texlist);
    tail_glow_color.a = GLOW_VARS(work)->alpha;
    njSetConstantMaterial(&tail_glow_color);
    njPushMatrix(NULL);
    njMultiMatrix(NULL, &chaos6_tail_matrix);
    njTranslate(NULL, tail_glow_x, tail_glow_y, tail_glow_z);
    TURN(njRotateY, tail_glow_ang_y);
    TURN(njRotateZ, tail_glow_ang_z);
    TURN(njRotateX, tail_glow_ang_x);
    draw_tree(&tail_glow_object);
    njPopMatrix(1);
}

static void chaos6_tail_glow_run(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case GLOW_IN:
        GLOW_VARS(work)->alpha += GLOW_FADE;
        if (GLOW_VARS(work)->alpha >= 1.0F) {
            GLOW_VARS(work)->alpha = 1.0F;
            work->mode = GLOW_HOLD;
        }
        break;
    case GLOW_HOLD:
        if (!tail_glow_on) {
            work->mode = GLOW_OUT;
        }
        break;
    case GLOW_OUT:
        GLOW_VARS(work)->alpha -= GLOW_FADE;
        if (GLOW_VARS(work)->alpha <= 0.0F) {
            task_kill(task);
            return;
        }
        break;
    }
    chaos6_tail_glow_draw(task);
}

void chaos6_tail_glow_start(struct task *task)
{
    tail_glow_on = TRUE;
    task->step = chaos6_tail_glow_run;
    task->draw = chaos6_tail_glow_draw;
}

void chaos6_tail_glow_end(void)
{
    tail_glow_on = FALSE;
}

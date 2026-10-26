/* the tornado's debris, which flies into it, and the spiral of bridge pieces
 * that circles it. the haze about the camera in the tornado's storm.
 * O_SETSMO, which puffs dust and sends debris to the tornado */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "draw/draw.h"
#include "draw/helpers.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "enemy/sai_textures.h"
#include "player/player.h"
#include "stg02/debris.h"
#include "stg02/tornado.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: stage_get_stage_act declared to return an int, where stage.h's
 * returns a Uint16. SHC then compares the result unextended (CMP/EQ R0,R8 at
 * 0x0c904b08). as stage.h declares it, 5 of windy_haze_step's 102 words
 * differ */
extern int stage_get_stage_act(void);

extern NJS_OBJECT sai_object;
extern NJS_MODEL dat_0c9bfa04;
extern NJS_MODEL dat_0c9bfb08;
extern NJS_MODEL dat_0c9bfc0c;
extern NJS_MODEL windy_haze_model;
extern NJS_OBJECT windy_bridge_piece_23_object;
extern NJS_OBJECT windy_bridge_piece_24_object;
extern NJS_OBJECT windy_bridge_piece_25_object;
extern NJS_OBJECT windy_bridge_piece_26_object;
extern NJS_OBJECT windy_bridge_piece_27_object;
extern NJS_OBJECT windy_bridge_piece_28_object;
extern NJS_OBJECT windy_bridge_piece_29_object;
extern NJS_OBJECT windy_bridge_piece_30_object;
extern NJS_OBJECT windy_bridge_piece_31_object;
extern NJS_OBJECT windy_bridge_piece_32_object;

/* each task here starts in MODE_START, which its first step leaves for
 * MODE_RUN. only the haze reaches MODE_END */
enum task_mode { MODE_START, MODE_RUN, MODE_END };

/* a piece of debris turns TUMBLE about x and about y a step */
#define TUMBLE 255
/* each step, debris moves twice its speed toward the tornado. its speed is
 * scl.x, plus DEBRIS_GAIN for each step it has flown */
#define DEBRIS_GAIN 0.12F
/* debris kills itself once within DEBRIS_REACH of the tornado across the
 * ground, or once its timer passes DEBRIS_STEPS */
#define DEBRIS_REACH 10.0F
#define DEBRIS_STEPS 150

/* the haze ends once player 0 is farther than HAZE_RANGE from the tornado's
 * eye. it turns HAZE_TURN degrees a step. on a step with a camera, its alpha
 * rises HAZE_FADE, to no more than 1 */
#define HAZE_RANGE 1500.0F
#define HAZE_TURN 10.0F
#define HAZE_FADE 0.12F

/* the spiral's pieces stand SPIRAL_RADIUS from the tornado across the
 * ground. each stands SPIRAL_SPACING degrees about it from the last, and
 * SPIRAL_RISE above it. the spiral turns SPIRAL_TURN degrees a step */
#define SPIRAL_PIECES 10
#define SPIRAL_RADIUS 100.0F
#define SPIRAL_SPACING 30
#define SPIRAL_RISE 5.0F
#define SPIRAL_TURN 5.0F
/* the spiral sinks SPIRAL_SINK for each degree it turns. each piece tilts
 * about x by the spiral's turn, plus SPIRAL_TILT degrees for each piece
 * before it */
#define SPIRAL_SINK 0.01F
#define SPIRAL_TILT 10

/* O_SETSMO puffs dust and sends debris each time its count passes
 * PUFF_WAIT */
#define PUFF_WAIT 15

struct haze_vars {
    Float alpha; /* the alpha it draws with, -1 at the start */
    Uint8 unk_04[2];
    Sint16 stage_act; /* the stage and act it started in */
    Float spin; /* its turn about y, in degrees */
};

struct spiral_vars {
    /* the spiral's turn about the tornado, in degrees, from 0 down */
    Float phase;
    Uint8 unk_04[4];
    NJS_OBJECT *piece; /* the piece model spiral_piece_draw draws next */
};

#define HAZE(work) ((struct haze_vars *)&(work)->vars)
#define SPIRAL(work) ((struct spiral_vars *)&(work)->vars)

static NJS_OBJECT *spiral_pieces[SPIRAL_PIECES] = {
    &windy_bridge_piece_23_object, &windy_bridge_piece_24_object,
    &windy_bridge_piece_25_object, &windy_bridge_piece_26_object,
    &windy_bridge_piece_27_object, &windy_bridge_piece_28_object,
    &windy_bridge_piece_29_object, &windy_bridge_piece_30_object,
    &windy_bridge_piece_31_object, &windy_bridge_piece_32_object,
};

static void debris_draw(struct task *task);
static void debris_draw_sai(struct task *task);
static void haze_draw(struct task *task);
static void spiral_piece_draw(struct task *task, Angle tilt, Angle facing,
                              Float height);

/* the first step sets scl.x at random, from 1 up to 2. it sets scl.y to the
 * height the debris starts at. it turns the debris to a random angle about x
 * and y */
void windy_tornado_debris_step(struct task *task)
{
    struct task_work *work = task->work;
    Float distance;
    Float speed;
    Float random;
    Angle ang;
    NJS_POINT3 target;

    switch (work->mode) {
    case MODE_START:
        work->mode = MODE_RUN;
        random = rand() * RAND_SCALE;
        work->scl.x = _builtin_fabsf(random) + 1.0F;
        work->scl.y = work->pos.y;
        ang = (Angle)(random * 360.0F * ANGLE_PER_DEGREE);
        work->ang.y = work->ang.x = ang;
        break;
    case MODE_RUN:
        TASK_TIMER(work)++;
        work->ang.x += TUMBLE;
        work->ang.y += TUMBLE;
        speed = work->scl.x + (DEBRIS_GAIN * TASK_TIMER(work));
        /* @bug uninitialized: windy_tornado_get_pos's result goes unread
         * (JSR at 0x0c904902). until the tornado's first run step, it leaves
         * target unwritten. only target.y is set (0x0c904912), so point_toward
         * reads target.x and z unset (JSR at 0x0c904924) */
        windy_tornado_get_pos(&target);
        target.y = work->scl.y;
        point_toward(&work->pos, &target, speed * 2.0F, &work->pos);
        if (work->scl.z == 1.0F) {
            debris_draw_sai(task);
        } else {
            debris_draw(task);
        }
        if (windy_tornado_distance(&work->pos, &distance) &&
            distance < DEBRIS_REACH) {
            task_kill(task);
            break;
        }
        if (TASK_TIMER(work) > DEBRIS_STEPS) {
            task_kill(task);
        }
        break;
    }
}

/* (int)scl.z picks the models: 0 draws all three, 1 the last two, and any
 * other value the last. the step draws sai_object instead where scl.z is
 * exactly 1. so only an scl.z between 1 and 2 draws the last two */
static void debris_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    switch ((int)work->scl.z) {
    case 0:
        draw_model(&dat_0c9bfb08);
        /* fall through */
    case 1:
        draw_model(&dat_0c9bfc0c);
        /* fall through */
    default:
        draw_model(&dat_0c9bfa04);
        break;
    }
    njPopMatrix(1);
}

static void debris_draw_sai(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(&e_sai_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&sai_object);
    njPopMatrix(1);
}

void windy_haze_step(struct task *task)
{
    static const NJS_POINT3 haze_tornado_eye = {3254.0F, -421.0F, -1665.0F};
    struct task_work *work = task->work;
    NJS_POINT3 eye;

    eye = haze_tornado_eye;
    switch (work->mode) {
    case MODE_START:
        work->mode = MODE_RUN;
        HAZE(work)->alpha = -1.0F;
        HAZE(work)->stage_act = stage_get_stage_act();
        break;
    case MODE_RUN:
        if (HAZE(work)->stage_act != stage_get_stage_act()) {
            work->mode = MODE_END;
            break;
        }
        HAZE(work)->spin += HAZE_TURN;
        if (camera_work != NULL) {
            work->pos.x = camera_work->pos.x;
            work->pos.y = camera_work->pos.y;
            work->pos.z = camera_work->pos.z;
            HAZE(work)->alpha += HAZE_FADE;
            if (HAZE(work)->alpha > 1.0F) {
                HAZE(work)->alpha = 1.0F;
            }
            haze_draw(task);
        }
        if (njDistanceP2P(&player_work[0]->pos, &eye) > HAZE_RANGE) {
            work->mode = MODE_END;
        }
        break;
    case MODE_END:
        task_kill(task);
        break;
    }
}

static void haze_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    Angle ang;

    texture_set_stage();
    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
    draw_set_material(HAZE(work)->alpha, 0.0F, 0.0F, 0.0F);
    pos = camera_work->pos;
    njPushMatrix(NULL);
    njTranslateV(NULL, &pos);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    TURN_VIA(njRotateX, camera_work->ang.x, ang);
    TURN_VIA(njRotateZ, camera_work->ang.z, ang);
    /* @bug arithmetic: spin grows without bound. from run step 1,179,648,
     * where it reaches 11,796,480 degrees, the FTRC at 0x0c904c84 converts
     * a value past int's range */
    TURN_VIA(njRotateY, (Angle)(HAZE(work)->spin * ANGLE_PER_DEGREE), ang);
    njScale(NULL, 1.0F, 1.2F, 1.0F);
    draw_model(&windy_haze_model);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    draw_attr_restore();
}

/* scl.y is the lowest piece's height before the spiral sinks. the first
 * step sets it at random, from -451 up to -441 */
void windy_tornado_spiral_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 center;
    Angle ang;
    Angle tilt;
    Angle turn;
    Float height;
    int i;

    switch (work->mode) {
    case MODE_START:
        work->scl.y = RANDOM(-451.0F, -441.0F);
        work->mode = MODE_RUN;
        break;
    case MODE_RUN:
        SPIRAL(work)->phase -= SPIRAL_TURN;
        /* @bug arithmetic: phase falls without bound. from run step
         * 2,359,297, past -11,796,480 degrees, the FTRC at 0x0c904dc8
         * converts a value past int's range */
        ang = (Angle)(SPIRAL(work)->phase * ANGLE_PER_DEGREE);
        work->pos.y = work->scl.y + (SPIRAL_SINK * SPIRAL(work)->phase);
        if (windy_tornado_get_pos(&center)) {
            for (i = 0; i < SPIRAL_PIECES; i++) {
                /* @bug arithmetic: tilt's FTRC converts a value past int's
                 * range from run step 2,359,297 + 2i: 0x0c904dfc for even i,
                 * 0x0c904e6e for odd */
                tilt = (Angle)((SPIRAL(work)->phase + (i * SPIRAL_TILT)) *
                               ANGLE_PER_DEGREE);
                /* @hack spelling: turn set inside njCos's argument, which keeps
                 * task in R10 and loads njSin's address for each call
                 * (0x0c904e20). set before the call, the function runs 32
                 * bytes long and 192 of its 203 words differ */
                work->pos.x =
                    center.x + (njCos(turn = (Angle)(i * SPIRAL_SPACING *
                                                     ANGLE_PER_DEGREE) +
                                             ang) *
                                SPIRAL_RADIUS);
                work->pos.z = center.z + (njSin(turn) * SPIRAL_RADIUS);
                height = work->pos.y + (i * SPIRAL_RISE);
                SPIRAL(work)->piece = spiral_pieces[i];
                spiral_piece_draw(task, tilt, ang, height);
            }
        }
        break;
    case MODE_END:
        break;
    }
}

static void spiral_piece_draw(struct task *task, Angle tilt, Angle facing,
                              Float height)
{
    struct task_work *work = task->work;
    NJS_OBJECT *piece = SPIRAL(work)->piece;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, height, work->pos.z);
    TURN(njRotateX, tilt);
    TURN(njRotateY, facing);
    draw_model(piece->model);
    njPopMatrix(1);
}

void windy_dust_source_step(struct task *task)
{
    static const NJS_VECTOR dust_speed = {5.0F, 4.0F, 2.0F};
    struct task_work *work = task->work;
    NJS_VECTOR speed;
    struct task *child;

    speed = dust_speed;
    switch (work->mode) {
    case MODE_START:
        work->mode = MODE_RUN;
        break;
    case MODE_RUN:
        work->tick++;
        speed.x += work->scl.x;
        speed.y += work->scl.x;
        speed.z += work->scl.x;
        if (work->tick > PUFF_WAIT) {
            work->tick = 0;
            dust_create(&work->pos, &speed, work->scl.y + 10.0F);
            if ((child = task_create(TASK_WORK, 5,
                                     windy_tornado_debris_step)) != NULL) {
                child->work->pos.x = work->pos.x;
                child->work->pos.y = work->pos.y;
                child->work->pos.z = work->pos.z;
            }
        }
        break;
    case MODE_END:
        break;
    }
}

/* Chaos 4's sickle. he lashes out, and the swamp rises where he stands. a
 * curved wave sweeps out from the rise toward player 0, growing as it goes.
 * while the next hit would beat him, a second wave follows */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos4/boss.h"
#include "b_chaos4/sickle.h"
#include "b_chaos4/swamp.h"
#include "b_chaos4/textures.h"
#include "chaos/chaos.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/motion.h"
#include "enemy/enemy.h"
#include "hud/life_gauge.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

/* the rise's two actions, which sickle_rise_draw plays at one frame, and
 * the wave's model */
extern NJS_ACTION chaos4_rise_action;
extern NJS_ACTION chaos4_rise_layer_action;
extern NJS_OBJECT chaos4_sickle_object;

enum sickle_mode { SICKLE_START, SICKLE_LASH, SICKLE_LASHED, SICKLE_END };

/* the rise's sound, held from the rise's last frame until the rise ends */
#define SE_RISE 0x176

#define SICKLE_Y 24.859F /* the height the rise and the wave stand at */

/* at LAST_HEALTH or less, the rise sends a second wave once its countdown
 * from SECOND_DELAY passes below 0 */
#define SECOND_DELAY 45

/* the wave's width, its work's scl.x, grows from 0 by SICKLE_GROWTH a
 * step, and the wave ends once the width reaches SICKLE_GROWN. width 1 is
 * its full width. there its SICKLE_SHAPES shapes sit SICKLE_REACH from its
 * position, and each has a radius of SICKLE_SHAPE_RADIUS, half its length.
 * both scale with the width */
#define SICKLE_GROWTH 0.01F
#define SICKLE_GROWN 1.3F
#define SICKLE_SHAPES 5
#define SICKLE_REACH 192.0F
#define SICKLE_SHAPE_RADIUS 26.0F
/* the reach sickle_place_shapes gives the wave's collision record, which
 * takes in the shapes wherever they sit */
#define SICKLE_RECORD_RADIUS 10000.0F
#define SICKLE_ATTACK 1 /* the attack level of the wave's shapes */

/* the wave's constant material at width 0, in red, green and blue:
 * -TINT_RED, -TINT_GREEN and TINT_BLUE. each shrinks with the width, to 0
 * at full width */
#define TINT_RED 0.14453125F
#define TINT_GREEN 1.0F
#define TINT_BLUE 1.0F

/* the vibration pack's frequency as the wave starts out */
#define SHAKE_FREQ 30

#define QUARTER_TURN NJM_DEG_ANG(90)
#define HALF_TURN NJM_DEG_ANG(180)

struct rise_vars {
    Uint8 unk_00[4];
    Sint32 timer; /* the countdown to the second wave */
    Float frame; /* the frame both rise actions are drawn at */
};

#define RISE_VARS(work) ((struct rise_vars *)&(work)->vars)

enum rise_mode { RISE_START, RISE_PLAY, RISE_SEND, RISE_WAIT, RISE_END };

enum sickle_wave_mode { WAVE_AIM, WAVE_GROW, WAVE_END };

static void sickle_rise_create(NJS_POINT3 *pos);
static void sickle_rise_step(struct task *task);
static void sickle_rise_draw(struct task *task);
static void sickle_create(NJS_POINT3 *pos);
static void sickle_step(struct task *task);
static void sickle_place_shapes(struct task_work *work);
static void sickle_draw(struct task *task);

/* each shape's heading off the wave's middle, in Ninja's angles: 0, then
 * about 17 degrees to either side, and twice that */
static const Float sickle_headings[SICKLE_SHAPES] = {
    0.0F, 3094.0F, -3094.0F, 6188.0F, -6188.0F,
};

/* the wave's boxes, which hurt players. they push nothing, and nothing
 * pushes them. sickle_place_shapes sets their places, angles and radii each
 * step */
static struct collision_shape sickle_shapes[SICKLE_SHAPES] = {
    {SHAPE_RECTANGLE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SICKLE_ATTACK,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     26.0F,
     5.0F,
     10.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SICKLE_ATTACK,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     26.0F,
     5.0F,
     10.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SICKLE_ATTACK,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     26.0F,
     5.0F,
     10.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SICKLE_ATTACK,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     26.0F,
     5.0F,
     10.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SICKLE_ATTACK,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     26.0F,
     5.0F,
     10.0F,
     {0, 0, 0},
     0,
     NULL},
};

/* he stops, turns his shapes off and lashes out, turning to player 0. the
 * swamp rises where he stands as the lash starts, and the mode ends once
 * his motion is back at rest */
void chaos4_sickle(struct task_work *work, struct task_motion *motion,
                   struct chaos *chaos)
{
    struct chaos4 *record = CHAOS4_VARS(work)->record;

    switch (work->submode) {
    case SICKLE_START:
        chaos4_set_shapes(work, SHAPES_OFF);
        chaos->motion.wanted = ACTION_LASH;
        record->move = MOVE_STILL;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        record->turn = TURN_PLAYER;
        work->submode = SICKLE_LASH;
    case SICKLE_LASH:
        if (chaos->motion.current == ACTION_LASH) {
            sickle_rise_create(&work->pos);
            swamp_ripple_add(&work->pos);
            sound_play(SE_LASH, 0, 0, 0);
            work->submode = SICKLE_LASHED;
        }
        break;
    case SICKLE_LASHED:
        work->submode = SICKLE_END;
        break;
    case SICKLE_END:
        if (chaos->motion.current == ACTION_REST) {
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        if (work->mode != BOSS_SWIM) {
            boss_next_mode = BOSS_SWIM;
            boss_last_mode = work->mode;
        }
        work->submode = SICKLE_START;
    }
}

/* make the rise at pos's x and z, at SICKLE_Y */
static void sickle_rise_create(NJS_POINT3 *pos)
{
    struct task *task;
    struct task_work *work;

    if ((task = task_create(TASK_WORK, 3, sickle_rise_step)) != NULL) {
        work = task->work;
        enemy_alloc_record(task, work);
        VECTOR_COPY(&work->pos, pos);
        work->pos.y = SICKLE_Y;
        task->step = sickle_rise_step;
        task->draw = sickle_rise_draw;
    }
}

/* the rise plays its actions a frame a step, up to chaos4_rise_action's
 * last frame, where it holds them and starts SE_RISE. it then sends the
 * wave, and at LAST_HEALTH or less a second one after SECOND_DELAY. then it
 * stops SE_RISE and ends */
static void sickle_rise_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case RISE_START:
        RISE_VARS(work)->frame = 0.0F;
        work->mode = RISE_PLAY;
    case RISE_PLAY:
        /* @hack spelling: the frame stepped in the test, which compares the
         * sum still in FR2 (FADD at 0x0c9031b6, FCMP/GT at 0x0c9031d0);
         * stepped ahead of it, sickle_rise_step runs 32 bytes long and 81 of
         * 108 words differ */
        if ((RISE_VARS(work)->frame += 1.0F) >=
            chaos4_rise_action.motion->nbFrame - 1.0F) {
            RISE_VARS(work)->frame = chaos4_rise_action.motion->nbFrame - 1.0F;
            work->mode = RISE_SEND;
            sound_play_held(SE_RISE, 0, 1, 0);
        }
        break;
    case RISE_SEND:
        sickle_create(&work->pos);
        if (life_gauge_health <= LAST_HEALTH) {
            RISE_VARS(work)->timer = SECOND_DELAY;
            work->mode = RISE_WAIT;
        } else {
            work->mode = RISE_END;
        }
        break;
    case RISE_WAIT:
        if (--RISE_VARS(work)->timer < 0) {
            sickle_create(&work->pos);
            work->mode = RISE_END;
        }
        break;
    case RISE_END:
        sound_stop(SE_RISE);
        task_kill(task);
        break;
    }
    sickle_rise_draw(task);
}

static void sickle_rise_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        njSetTexture(&chaos4_kama_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        draw_action(&chaos4_rise_action, RISE_VARS(work)->frame);
        draw_action(&chaos4_rise_layer_action, RISE_VARS(work)->frame);
        njPopMatrix(1);
    }
}

/* make the wave at pos's x and z, at SICKLE_Y */
static void sickle_create(NJS_POINT3 *pos)
{
    struct task *task;
    struct task_work *work;

    if ((task = task_create(TASK_WORK, 3, sickle_step)) != NULL) {
        work = task->work;
        enemy_alloc_record(task, work);
        VECTOR_COPY(&work->pos, pos);
        work->pos.y = SICKLE_Y;
        collision_init(task, sickle_shapes, COLLISION_OBJECT, 1, SICKLE_SHAPES);
        task->step = sickle_step;
        task->draw = sickle_draw;
    }
}

/* on its first step, the wave shakes the vibration pack by its distance
 * from player 0. vibration_start clamps each power above 4 to 4, so the
 * shake is at 4 within 80 and at 3 farther. the wave turns to face player
 * 0, grows each step, and ends once it reaches SICKLE_GROWN */
static void sickle_step(struct task *task)
{
    struct task_work *work = task->work;
    Float distance;
    int power;

    switch (work->mode) {
    case WAVE_AIM:
        distance = njSqrt(SQUARE(work->pos.x - player_work[0]->pos.x) +
                          SQUARE(work->pos.z - player_work[0]->pos.z));
        if (distance < 20.0F) {
            power = 11;
        } else if (distance < 40.0F) {
            power = 9;
        } else if (distance < 60.0F) {
            power = 7;
        } else if (distance < 80.0F) {
            power = 5;
        } else {
            power = 3;
        }
        vibration_start(0, power, SHAKE_FREQ, 1);
        work->ang.y = (Angle)(atan2f(player_work[0]->pos.z - work->pos.z,
                                     player_work[0]->pos.x - work->pos.x) *
                              ANGLE_PER_RADIAN);
        work->scl.x = 0.0F;
        work->mode = WAVE_GROW;
    case WAVE_GROW:
        work->scl.x += SICKLE_GROWTH;
        if (work->scl.x >= SICKLE_GROWN) {
            work->mode = WAVE_END;
        }
        sickle_place_shapes(work);
        break;
    case WAVE_END:
        task_kill(task);
        break;
    }
    sickle_draw(task);
}

/* place each shape on the wave's arc, the width times SICKLE_REACH from the
 * wave's position, at its heading in sickle_headings off the wave's. each
 * shape turns to lie along the arc, and its radius is the width times
 * SICKLE_SHAPE_RADIUS. then register the record */
static void sickle_place_shapes(struct task_work *work)
{
    NJS_POINT3 reach;
    NJS_POINT3 *center;
    struct collision_shape *shape;
    Float turn;
    int i;

    reach.x = work->scl.x * SICKLE_REACH;
    reach.y = reach.z = 0.0F;
    for (i = 0; i < SICKLE_SHAPES; i++) {
        shape = &work->collision->shapes[i];
        center = &shape->center;
        njPushMatrix(&_unit_matrix_);
        TURN_VIA(njRotateY, sickle_headings[i] - work->ang.y, turn);
        njCalcPoint(NULL, &reach, center);
        njPopMatrix(1);
        VECTOR_SUM(center, &work->pos, center);
        /* @hack spelling: the angle stored through work->collision->shapes, not
         * shape, which loads the shape's address again for the store
         * (0x0c90359c to 0x0c9035a4, MOV.L R3,@(36,R2) at 0x0c9035b8).
         * through shape, sickle_place_shapes runs 96 bytes short, and 432 of
         * 457 words differ */
        work->collision->shapes[i].ang.y =
            (Angle)(sickle_headings[i] - work->ang.y + QUARTER_TURN);
        shape->radius = work->scl.x * SICKLE_SHAPE_RADIUS;
        work->collision->radius = SICKLE_RECORD_RADIUS;
    }
    collision_register(work);
}

/* on a frame's first step, draw the wave at its width. past full width,
 * its constant material falls from 0 to -1 in each color by SICKLE_GROWN */
static void sickle_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    Float fade;

    if (frame_step == 0) {
        njSetTexture(&chaos4_kama_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, HALF_TURN - work->ang.y, ang);
        njScale(NULL, work->scl.x, 1.0F, work->scl.x);
        if (work->scl.x <= 1.0F) {
            draw_set_material(0.0F, -((1.0F - work->scl.x) * TINT_RED),
                              -((1.0F - work->scl.x) * TINT_GREEN),
                              (1.0F - work->scl.x) * TINT_BLUE);
        } else {
            fade = -1.0F + ((SICKLE_GROWN - work->scl.x) *
                            (1.0F / (SICKLE_GROWN - 1.0F)));
            draw_set_material(0.0F, fade, fade, fade);
        }
        njDrawObject(&chaos4_sickle_object);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPopMatrix(1);
    }
}

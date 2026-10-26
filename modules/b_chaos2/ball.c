/* Chaos 2 as a ball of water. his figure rounds into the ball and back,
 * and the ball leaps at player 0. it also bursts into drops, which crawl
 * back to him while a core hovers */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos2/ball.h"
#include "b_chaos2/beads.h"
#include "b_chaos2/blight.h"
#include "b_chaos2/boss.h"
#include "b_chaos2/effects.h"
#include "b_chaos2/light.h"
#include "b_chaos2/ripple.h"
#include "b_chaos2/textures.h"
#include "b_chaos2/trail.h"
#include "chaos/chaos.h"
#include "chaos/effect.h"
#include "chaos/emeralds.h"
#include "chaos/puddle.h"
#include "chaos/steer.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/motion.h"
#include "draw/tree.h"
#include "effect/brain.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

extern NJS_OBJECT chaos2_core_object;

#define SE_ROUND 0x157
#define SE_BOUNCE 0x158
#define SE_BURST 0x15a
#define SE_DROP_BACK 0x15b
#define SE_DROP_THROWN 0x15c

/* the count of chaos2_round_object's points */
#define ROUND_POINTS 661
/* the ball takes a point d from the model's middle along the line from the
 * middle through it. it takes the point d * d * ROUND_SHARE * ROUND_RADIUS
 * out, but no farther than ROUND_RADIUS, 2 inside the ball's radius */
#define ROUND_SHARE 0.02F
#define ROUND_RADIUS (BALL_RADIUS - 2.0F)
/* the steps the rounding blends over */
#define ROUND_TIME 90
/* past a blend of ROUND_FADE_AT, the figure fades out. as it rounds, it
 * moves ROUND_SHIFT along its own x */
#define ROUND_FADE_AT 0.7F
#define ROUND_SHIFT 6.0F

/* how round_blend_points moves the model's points */
enum round_mode {
    ROUND_FIND, /* find where the ball takes each point, then blend */
    ROUND_BLEND, /* blend each point between its own place and the ball's */
    ROUND_STILL /* move none */
};

/* where the ball takes a point */
struct round_point {
    NJS_POINT3 pos;
    Uint8 unk_0c[4];
};

/* the ball leaps BOUNCE_HEIGHT high, and at most BOUNCE_REACH across.
 * while the boss has CHAOS2_STRUCK, it leaps BOUNCE_REACH, within
 * BOUNCE_SPREAD degrees of the way to player 0 */
#define BOUNCE_HEIGHT 20.0F
#define BOUNCE_REACH 50.0F
#define BOUNCE_SPREAD 40.0F
/* the share of its scale the ball squashes and stretches by as it leaps,
 * lands or is struck. the wobble then dies away */
#define BOUNCE_WOBBLE 0.22F
/* the ball drops to rest once its edge comes this far from the hall's
 * middle */
#define BOUNCE_EDGE 100.0F
/* the ball leaps again from each landing until BOUNCE_TIME steps have
 * passed */
#define BOUNCE_TIME 600

#define DROPS 24
/* a drop hops off within DROP_SPREAD degrees of the way to player 0 */
#define DROP_SPREAD 90.0F
/* a drop stops and falls straight down once this far from the hall's
 * middle */
#define DROP_REACH 90.0F
/* a drop lands for good as its landings reach DROP_LANDINGS. setting its
 * landings to DROP_LANDINGS makes the next landing its last */
#define DROP_LANDINGS 5
/* a thrown drop reaches the core in THROW_STEPS steps */
#define THROW_STEPS 30.0F
/* a crawling drop moves CRAWL_SPEED a step, and is back once within
 * BACK_NEAR of the boss */
#define CRAWL_SPEED 0.25F
#define BACK_NEAR 10.0F
/* a drop struck while player 0 faces away from the boss is knocked
 * KNOCK_REACH across, the way player 0 faces */
#define KNOCK_REACH 50.0F

/* the record at a drop's task->extra */
struct drop {
    Angle heading; /* the heading it hops along */
    Uint8 unk_04[4];
    Sint8 bounces; /* its landings so far */
    Angle phase; /* its wobble's phase */
    Float wobble; /* the share its scale wobbles by, which dies away */
    Float size; /* its size, which its shapes take as their radius */
};

struct drop_vars {
    /* the frame of the ball sprite, which chaos2_ball_sprite_draw reads */
    Float frame;
};

#define DROP(work) ((struct drop_vars *)&(work)->vars)

/* a drop's work->mode. a drop hops and falls until it lands for good, then
 * crawls back to the boss and shrinks away */
enum drop_mode {
    DROP_START,
    DROP_HOP,
    DROP_FLY,
    DROP_FALL,
    DROP_THROW, /* at the core, struck while player 0 faces the boss */
    DROP_THROWN,
    DROP_LAND,
    DROP_CRAWL, /* back to the boss */
    DROP_BACK,
    DROP_SHRINK
};

/* bits of a drop's work flags, which drop_step clears and sets again each
 * step. it sets DROP_ALOFT in the modes up to DROP_THROWN, and drop_draw
 * then draws a shadow. it sets DROP_LANDED from DROP_LAND on, and drop_draw
 * then draws the drop as chaos_blob */
#define DROP_ALOFT 0x100
#define DROP_LANDED 0x200

struct core_vars {
    Uint8 unk_00[4];
    /* @hack type: the steps since the core's last brain, in a union whose all
     * clears them and the half word after them in one store (MOV.L
     * R10,@(12,R14) at 0x0c905328); as a lone Sint16 cleared alone,
     * core_step runs long and 199 of 218 words differ */
    union {
        Sint16 steps;
        Sint32 all;
    } brain;
};

#define CORE(work) ((struct core_vars *)&(work)->vars)

/* the core's work->mode */
enum core_mode {
    CORE_START,
    CORE_RISE, /* to CORE_LOW */
    /* from CORE_LOW to CORE_HIGH, rising while player 0 is near and
     * sinking while he isn't */
    CORE_HOVER,
    CORE_DROP, /* hop, then fall */
    CORE_FALL
};

/* the core hovers from CORE_LOW to CORE_HIGH. player 0 is near while he is
 * within CORE_NEAR of it, and less than CORE_NEAR below it */
#define CORE_LOW 30.0F
#define CORE_HIGH 60.0F
#define CORE_NEAR 30.0F
/* the core rises or sinks CORE_SPEED a step, and climbs CORE_CLIMB a step
 * while player 0 is near. SHC reads these literals as the image's words,
 * 0x3e999998 and 0x3fbffffe, which lie just under 0.3 and 1.5 */
#define CORE_SPEED 0.29999998F
#define CORE_CLIMB 1.4999998F
/* while it rises or hovers, the core makes a brain each time its count
 * passes BRAIN_WAIT, so every 31 steps */
#define BRAIN_WAIT 30

/* the burst starts a blight at once and every BLIGHT_WAIT + 1 steps after.
 * it spaces its lights by LIGHT_WAIT the same way, but starts only one */
#define BLIGHT_WAIT 60
#define LIGHT_WAIT 60

/* the shadow's eight texanims, on chaos2_effect_texlist's textures 17 to
 * 24. chaos2_shadow_draw draws only the first */
static NJS_TEXANIM shadow_frames[8] = {
#include "b_chaos2/ball/shadow_frames.inc"
};
static NJS_SPRITE shadow_sprite = {
#include "b_chaos2/ball/shadow_sprite.inc"
};
/* set at the first rounding, once kept_points holds the model's points. each
 * rounding then moves the model's points, so later roundings keep that first
 * copy */
static Sint8 figure_kept = 0;
/* the offset material chaos2_round draws his figure in. a negative alpha
 * fades it */
static NJS_ARGB round_color = {0.0F, 0.0F, 0.0F, 0.0F};
/* the drops' offset material, all 0, which leaves their colors as they
 * are */
static NJS_ARGB drop_color = {0.0F, 0.0F, 0.0F, 0.0F};
/* a drop's two shapes. the first hurts players. the second hurts enemies,
 * and takes the first's place while the drop is thrown */
static struct collision_shape drop_shapes[2] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     0.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_OFF,
     ATTACK(1) | SHAPE_HURTS_ENEMIES,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     0.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};
/* a drop's start from the ball's middle, before drop_step turns it. drop_step
 * sets its x */
static NJS_VECTOR drop_offset = {0.0F, 0.0F, 0.0F};
/* the core's shape. a strike on it calls the drops back */
static struct collision_shape core_shape[1] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_HURTS_ENEMIES,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     5.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

static enum round_mode round_mode;
static Sint8 drops_left; /* the drops not yet back */
/* set as a drop or the core joins the puddle. the burst then shows the
 * puddle, wobbles it, and clears this */
static Sint8 puddle_joined;
/* set once the core is struck. the drops then fall and crawl back, and the
 * core blinks */
static Sint8 drops_recalled;
/* the core's work, which the core sets as it starts. nothing clears it
 * when the core ends */
static struct task_work *core_work;
/* the figure's own points, and where the ball takes each */
static NJS_POINT3 kept_points[ROUND_POINTS];
static struct round_point round_points[ROUND_POINTS];
/* how far the rounding has come, from 0 for his figure to 1 for the ball,
 * and how far it comes a step */
static Float round_blend;
static Float round_step;
/* the phase and the size of the bouncing ball's wobble, then of the
 * puddle's wobble in the burst */
static Angle bounce_phase;
static Float bounce_wobble;
static Angle burst_phase;
static Float burst_wobble;
/* the blights the burst has yet to start and the steps to the next, and
 * the same for its light */
static Sint16 blights_left;
static Sint16 blight_wait;
static Sint16 lights_left;
static Sint16 light_wait;

/* move object's points as round_mode says. ROUND_BLEND puts each point blend
 * of the way from its kept place to its place on the ball times scale */
static void round_blend_points(NJS_OBJECT *object, Float blend, Float scale)
{
    NJS_POINT3 *point;
    NJS_POINT3 *kept;
    struct round_point *round;
    Float length;
    Float radius;
    Float base;
    Float weighted;
    int i;

    switch (round_mode) {
    case ROUND_FIND:
        point = object->model->points;
        kept = kept_points;
        round = round_points;
        for (i = 0; i < ROUND_POINTS; i++) {
            round->pos = *point;
            njUnitVector(&round->pos);
            length = njScalor(point);
            length = length * length * ROUND_SHARE;
            radius = length * ROUND_RADIUS < ROUND_RADIUS
                         ? length * ROUND_RADIUS
                         : ROUND_RADIUS;
            round->pos.x *= radius;
            round->pos.y *= radius;
            round->pos.z *= radius;
            if (!figure_kept) {
                *kept++ = *point;
            }
            round++;
            point++;
        }
        figure_kept = 1;
        round_mode = ROUND_BLEND;
    case ROUND_BLEND:
        weighted = blend * scale;
        base = 1.0F - blend;
        kept = kept_points;
        point = object->model->points;
        round = round_points;
        for (i = 0; i < ROUND_POINTS; i++) {
            point->x = (round->pos.x * weighted) + (kept->x * base);
            point->y = (round->pos.y * weighted) + (kept->y * base);
            point->z = (round->pos.z * weighted) + (kept->z * base);
            point++;
            kept++;
            round++;
        }
        break;
    case ROUND_STILL:
        break;
    }
}

void chaos2_round(struct task_work *work, struct chaos *chaos,
                  struct chaos *record)
{
    Uint32 control;
    Angle ang;

    switch (work->submode) {
    case 0:
        record->motion.wanted = ACTION_STAND;
        work->submode = 1;
    case 1:
        if (record->motion.current != ACTION_STAND) {
            break;
        }
        if (work->mode == BOSS_TO_BALL) {
            round_blend = 0.0F;
            round_step = 1.0F / ROUND_TIME;
            chaos->figures &= ~FIGURE_BODY;
            chaos->figures |= FIGURE_BALL;
            brain_shown = 0;
            chaos_emerald_create(&boss_points[10].pos, 0);
            chaos_emerald_create(&boss_points[11].pos, 1);
            chaos->flags |= CHAOS_NO_BEADS;
        } else {
            round_blend = 1.0F;
            round_step = -1.0F / ROUND_TIME;
        }
        CHAOS2_VARS(work)->timer = ROUND_TIME;
        round_mode = ROUND_FIND;
        /* a blend of -1 puts each point at twice its own place less the
         * ball's, and this step draws his figure so */
        round_blend_points(chaos2_round_object, -1.0F, 1.0F);
        chaos2_set_shapes(work, SHAPES_CORE);
        work->collision->shapes[SHAPE_CORE].radius = 15.0F;
        /* the same row again, which changes nothing: chaos2_set_shapes
         * writes no radius */
        chaos2_set_shapes(work, SHAPES_CORE);
        work->collision->shapes[SHAPE_CORE].center.y = BALL_RADIUS;
        work->submode = 2;
        sound_play_at(SE_ROUND, 0, 0, 0, work->pos.x, work->pos.y, work->pos.z);
        chaos->scale.x = chaos->scale.y = chaos->scale.z = round_blend;
        break;
    case 2:
        round_blend += round_step;
        round_blend_points(chaos2_round_object, round_blend, 1.0F);
        chaos->scale.x = chaos->scale.y = chaos->scale.z = round_blend;
        --CHAOS2_VARS(work)->timer;
        if (CHAOS2_VARS(work)->timer == 0) {
            work->submode = MODE_DONE;
        }
        work->collision->shapes[SHAPE_CORE].radius =
            chaos->scale.x * BALL_RADIUS;
        break;
    }
    if (work->submode == MODE_DONE) {
        if (work->mode == BOSS_FROM_BALL) {
            chaos->figures |= FIGURE_BODY;
            brain_shown = 1;
            chaos->figures &= ~FIGURE_BALL;
            chaos_emerald_return_all();
            chaos->flags &= ~CHAOS_NO_BEADS;
        }
        round_mode = ROUND_STILL;
        /* under ROUND_STILL, this moves no point */
        round_blend_points(chaos2_round_object, 2.0F, 1.0F);
        if (boss_return_mode != work->mode) {
            boss_next_mode = boss_return_mode;
            boss_last_mode = work->mode;
        }
        return;
    }
    draw_set_palette(4);
    njPushMatrix(NULL);
    control = _nj_control_3d_flag_;
    njControl3D(_nj_control_3d_flag_ | NJD_CONTROL_3D_OFFSET_MATERIAL);
    njSetTexture(chaos->texlist);
    njSetConstantMaterial(&round_color);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, -work->ang.y + 0x8000, ang);
    /* past ROUND_FADE_AT, the figure's alpha falls from 0 to -1 */
    if (round_blend > ROUND_FADE_AT) {
        round_color.a =
            (ROUND_FADE_AT - round_blend) * (1.0F / (1.0F - ROUND_FADE_AT));
    } else {
        /* @hack spelling: the alpha through round_color's address, so each arm
         * loads that address for itself (MOV.L at 0x0c9041fa and
         * 0x0c904260); as round_color.a, SHC loads it once, ahead of the
         * test, and 92 of 342 words differ */
        (&round_color)->a = 0.0F;
    }
    njSetTexture(chaos->texlist);
    njSetTextureNum(4);
    njSetConstantMaterial(&round_color);
    njTranslate(NULL, round_blend * ROUND_SHIFT,
                chaos_params->model_y +
                    (round_blend * (BALL_RADIUS - chaos_params->model_y)),
                0.0F);
    draw_tree(chaos2_round_object);
    round_color.a = 0.0F;
    njSetConstantMaterial(&round_color);
    njControl3D(control);
    njPopMatrix(1);
    draw_set_palette(0);
}

void chaos2_bounce(struct task *task, struct task_motion *motion,
                   struct chaos *chaos)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    Float launch;
    Float steps;
    Float speed;
    Float x;
    Float z;
    Float distance;
    Float pos_x;
    Float squared;
    Float wave;
    Angle heading;

    switch (work->submode) {
    case 0:
        chaos->figures |= FIGURE_BALL;
        CHAOS2_VARS(work)->timer = 0;
        chaos2_set_shapes(work, SHAPES_CORE);
        work->collision->shapes[SHAPE_CORE].radius = BALL_RADIUS;
        work->collision->shapes[SHAPE_CORE].center.y = BALL_RADIUS;
        /* @hack spelling: this update and the bounce's other two spelled out,
         * each naming the core shape twice, which keeps the shape's offset,
         * 0x2a4, in R8 (MOV.W at 0x0c904362); as &= and |=, 200 of 528 words
         * differ */
        work->collision->shapes[SHAPE_CORE].levels =
            work->collision->shapes[SHAPE_CORE].levels & ~SHAPE_GUARD;
        chaos->flags |= CHAOS_NO_BEADS;
    case 1:
        /* launch is the upward speed that peaks at BOUNCE_HEIGHT. steps is
         * how many steps the leap takes, up and back down */
        launch = njSqrt(2.0F * BOUNCE_HEIGHT * GRAVITY);
        steps = launch * (2.0F / GRAVITY);
        x = player->pos.x - work->pos.x;
        z = player->pos.z - work->pos.z;
        if (chaos->flags & CHAOS2_STRUCK) {
            speed = BOUNCE_REACH / steps;
            heading = (Angle)(atan2f(z, x) * ANGLE_PER_RADIAN);
            heading += (Angle)((BOUNCE_SPREAD -
                                (rand() * (2 * BOUNCE_SPREAD * RAND_SCALE))) *
                               ANGLE_PER_DEGREE);
            motion->speed.x = speed * njCos(heading);
            motion->speed.z = speed * njSin(heading);
        } else {
            distance = njSqrt((x * x) + (z * z));
            speed = (distance < BOUNCE_REACH ? distance : BOUNCE_REACH) /
                    (steps * distance);
            x *= speed;
            z *= speed;
            motion->speed.x = x;
            motion->speed.z = z;
        }
        motion->speed.y = launch;
        sound_play_at(SE_BOUNCE, 0, 0, 0, work->pos.x, work->pos.y,
                      work->pos.z);
        /* @hack spelling: the wobble set inside the phase's store, which pools
         * bounce_phase's address first and keeps it in R9, and
         * bounce_wobble's in R10 (0x0c90436a, 0x0c90436c); as two
         * statements, 14 of 528 words differ */
        bounce_phase = (bounce_wobble = BOUNCE_WOBBLE, 0);
        work->submode = 2;
    case 2:
        motion->speed.y -= GRAVITY;
        njAddVector(&work->pos, &motion->speed);
        if (work->pos.y < chaos->ground) {
            work->pos.y = chaos->ground;
            work->submode = 4;
            chaos_effect_splash(work, 48, 1.0F, 0.0F);
            bounce_wobble = BOUNCE_WOBBLE;
            bounce_phase = 0;
            chaos2_beads_create(task, 10, 0.2F);
            chaos_create_task(2, chaos2_ripple_step, task, NULL);
            sound_play_at(SE_BOUNCE, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
            vibration_start(0, 3, 15, 0);
        }
        if (work->flags & COLLISION_DAMAGED) {
            /* struck, the ball flies away from player 0, guarded, and its
             * leaps end */
            heading = chaos_angle_to_player(work, 0) + 0x8000;
            motion->speed.y = 30.0F * GRAVITY;
            motion->speed.x = njCos(heading) * (50.0F * (1.0F / 30.0F));
            motion->speed.z = njSin(heading) * (50.0F * (1.0F / 30.0F));
            bounce_wobble = BOUNCE_WOBBLE;
            bounce_phase = 0;
            work->collision->shapes[SHAPE_CORE].levels =
                (work->collision->shapes[SHAPE_CORE].levels & ~SHAPE_GUARD) |
                SHAPE_GUARD;
            CHAOS2_VARS(work)->timer = BOUNCE_TIME;
            sound_play_at(SE_BOUNCE, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
            vibration_start(0, 4, 45, 2);
        }
        /* @hack spelling: pos.x through a local, which loads it into FR4 and
         * pos.z into FR5 (0x0c90467e, 0x0c904682); read in place, 6 of 528
         * words differ */
        pos_x = work->pos.x;
        squared = (pos_x * pos_x) + (work->pos.z * work->pos.z);
        if (squared >= SQUARE(BOUNCE_EDGE - BALL_RADIUS)) {
            motion->speed.x = motion->speed.z = 0.0F;
            motion->speed.y = -10.0F * GRAVITY;
            work->submode = 3;
            sound_play_at(SE_BOUNCE, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        break;
    case 4:
        work->collision->shapes[SHAPE_CORE].levels =
            work->collision->shapes[SHAPE_CORE].levels & ~SHAPE_GUARD;
        if (CHAOS2_VARS(work)->timer < BOUNCE_TIME) {
            work->submode = 1;
        } else {
            work->submode = 3;
        }
        break;
    case 3:
        /* the ball bounces in place, losing speed, until it comes to
         * rest */
        motion->speed.y -= GRAVITY;
        work->pos.y += motion->speed.y;
        if (work->pos.y < chaos->ground) {
            sound_play_at(SE_BOUNCE, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
            work->pos.y = chaos->ground;
            chaos2_beads_create(task, 10, 0.2F);
            chaos_create_task(2, chaos2_ripple_step, task, NULL);
            motion->speed.y *= -0.6F;
            if (motion->speed.y < 1.0F) {
                work->submode = MODE_DONE;
            } else {
                bounce_wobble = BOUNCE_WOBBLE;
                bounce_phase = 0;
            }
        }
        break;
    }
    CHAOS2_VARS(work)->timer++;
    bounce_phase += NJM_DEG_ANG(10);
    bounce_wobble *= 0.99F;
    wave = bounce_wobble * njSin(bounce_phase);
    /* @hack redundant: a test of wave that never runs, which puts 1.0 in FR15,
     * and x and z in FR14 and FR13 (0x0c904378, 0x0c904436), as the image
     * has them; without it, the three registers rotate and 219 of 528 words
     * differ */
    if (0) {
        if (wave < 1.0F) {
        }
    }
    chaos->scale.x = chaos->scale.z = 1.0F + wave;
    chaos->scale.y = 1.0F - wave;
    chaos2_beads_create(task, 2, 0.08F);
    if (work->submode == MODE_DONE) {
        task_destroy_children(task);
        work->pos.y = 0.0F;
        work->ang.y = chaos_angle_to_player(work, 0);
        if (work->mode != BOSS_FROM_BALL) {
            boss_next_mode = BOSS_FROM_BALL;
            boss_last_mode = work->mode;
        }
        boss_return_mode = BOSS_CHASE;
        chaos->flags &= ~CHAOS2_STRUCK;
    }
}

static void drop_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct drop *drop = task->extra;
    NJS_VECTOR scale;
    Float size;

    if (work->flags & DROP_ALOFT) {
        chaos2_shadow_draw(&work->pos, 4.0F);
    }
    draw_set_palette(4);
    draw_control_save();
    njControl3D(NJD_CONTROL_3D_OFFSET_MATERIAL);
    njSetTexture(&chaos2_texlist);
    njSetTextureNum(4);
    njSetConstantMaterial(&drop_color);
    njPushMatrix(NULL);
    size = drop->size * 0.4F;
    scale.x = scale.z = size * (1.0F - (drop->wobble * njCos(drop->phase)));
    scale.y = size * (1.0F - (drop->wobble * njSin(drop->phase)));
    if (work->flags & DROP_LANDED) {
        njTranslateV(NULL, &work->pos);
        njScaleV(NULL, &scale);
        draw_tree(&chaos_blob);
    } else {
        njTranslate(NULL, work->pos.x, work->pos.y + drop->size, work->pos.z);
        njScaleV(NULL, &scale);
        draw_tree(&chaos2_drop_object);
        chaos2_ball_sprite_draw(work);
    }
    njPopMatrix(1);
    draw_control_restore();
    /* drop_draw makes no draw_attr_save of its own, so this puts back what
     * the last save kept */
    draw_attr_restore();
    draw_set_palette(0);
}

static void drop_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct drop *drop = task->extra;
    Float root;
    Angle ang_y;
    Angle ang_z;
    Float height;
    Float steps;
    Float distance;
    Float speed;
    Float squared;
    NJS_VECTOR offset;

    work->flags &= ~(DROP_ALOFT | DROP_LANDED);
    switch (work->mode) {
    case DROP_START:
        njPushMatrix(&_unit_matrix_);
        ang_y = drop->heading;
        ang_z = (Angle)(rand() * ((360.0F * RAND_SCALE) * ANGLE_PER_DEGREE));
        TURN(njRotateZ, ang_z);
        TURN(njRotateY, ang_y);
        drop_offset.x = RANDOM(12.0F, 17.0F);
        njCalcVector(NULL, &drop_offset, &offset);
        njAddVector(&work->pos, &offset);
        njPopMatrix(1);
        root = rand() * (1.5F * RAND_SCALE);
        drop->size = (root * root) + 2.5F;
        chaos_effect_splash(work, 8, 4.0F, 0.0F);
        collision_init(task, drop_shapes, COLLISION_ITEM, 1, 2);
        work->collision->shapes[0].radius = drop->size;
        work->collision->shapes[1].radius = drop->size;
        DROP(work)->frame = 0.0F;
        task->draw = drop_draw;
    case DROP_HOP:
        height = RANDOM(10.0F, 20.0F);
        motion->speed.y = njSqrt(height * (2.0F * GRAVITY));
        steps = motion->speed.y * (2.0F / GRAVITY);
        /* @hack spelling: the reach divided in a statement of its own, which
         * keeps rand's address in R8 and chaos_effect_splash's in R10
         * (0x0c904b08, 0x0c904b04); in one expression, 66 of 696 words
         * differ */
        speed = RANDOM(10.0F, 20.0F);
        speed /= steps;
        motion->speed.x = speed * njCos(drop->heading);
        motion->speed.z = speed * njSin(drop->heading);
        drop->phase = 0;
        drop->wobble = 0.2F;
        /* @bug logic: drop_step clears DROP_ALOFT before its switch
         * (0x0c904b0e), so this test and DROP_LAND's never hold (TST at
         * 0x0c904c96 and 0x0c905044), and neither sound plays */
        if (work->flags & DROP_ALOFT) {
            sound_play_at(SE_BOUNCE, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        work->mode = DROP_FLY;
    case DROP_FLY:
        squared = ACROSS_SQUARED(&work->pos);
        if (squared >= SQUARE(DROP_REACH) || drops_recalled) {
            chaos_effect_splash(work, 8, 1.2F, 0.0F);
            motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
            drop->bounces = DROP_LANDINGS;
            work->mode = DROP_FALL;
        }
        if (work->flags & COLLISION_DAMAGED) {
            Angle off;

            chaos_effect_splash(work, 8, 1.2F, 0.0F);
            /* @bug logic: the difference is never wrapped into a half
             * turn (SUB at 0x0c904d90). where the two headings lie across
             * the wrap, player 0 facing the boss counts as facing away */
            off = player_work[0]->ang.y - chaos_angle_to_player(boss_work, 0) -
                  0x8000;
            if ((off < 0 ? -off : off) < NJM_DEG_ANG(45)) {
                work->mode = DROP_THROW;
            } else {
                height = RANDOM(10.0F, 20.0F);
                motion->speed.y = njSqrt(height * (2.0F * GRAVITY));
                steps = motion->speed.y * (2.0F / GRAVITY);
                speed = KNOCK_REACH / steps;
                drop->heading = player_work[0]->ang.y;
                motion->speed.x = speed * njCos(drop->heading);
                motion->speed.z = speed * njSin(drop->heading);
                drop->phase = 0;
                drop->wobble = 0.2F;
                drop->bounces = DROP_LANDINGS;
                work->mode = DROP_FLY;
            }
        }
    case DROP_FALL:
        motion->speed.y -= GRAVITY;
        njAddVector(&work->pos, &motion->speed);
        if (work->pos.y <= 0.0F) {
            drop->bounces++;
            if (drop->bounces >= DROP_LANDINGS) {
                chaos_effect_splash(work, 8, drop->size * 0.6F, 0.0F);
                work->mode = DROP_LAND;
            } else {
                chaos_effect_splash(work, 6, drop->size * 0.3F, 0.0F);
                work->mode = DROP_HOP;
            }
        }
        break;
    case DROP_THROW:
        work->collision->kind = COLLISION_ATTACK;
        /* @bug memory: reads core_work->pos after the core's work is freed,
         * by the MOV.Ls at 0x0c904f10 to 0x0c904f18. CORE_FALL destroys the
         * core's task, and core_work still points at its work. once more than
         * half the drops are back, the core falls unstruck. a drop still
         * hopping can then be thrown, and it aims at that freed work */
        motion->speed = core_work->pos;
        njSubVector(&motion->speed, &work->pos);
        motion->speed.x *= 1.0F / THROW_STEPS;
        motion->speed.y *= 1.0F / THROW_STEPS;
        motion->speed.z *= 1.0F / THROW_STEPS;
        work->collision->shapes[0].flags |= SHAPE_OFF;
        work->collision->shapes[1].flags &= ~SHAPE_OFF;
        sound_play_at(SE_DROP_THROWN, 0, 0, 0, work->pos.x, work->pos.y,
                      work->pos.z);
        work->mode = DROP_THROWN;
    case DROP_THROWN:
        njAddVector(&work->pos, &motion->speed);
        chaos_create_task(2, chaos2_trail_step, task, NULL);
        squared = ACROSS_SQUARED(&work->pos);
        if (squared >= SQUARE(DROP_REACH) || work->pos.y > 60.0F) {
            chaos_effect_splash(work, 8, 1.2F, 0.0F);
            motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
            drop->bounces = DROP_LANDINGS;
            work->collision->shapes[0].flags &= ~SHAPE_OFF;
            work->collision->shapes[1].flags |= SHAPE_OFF;
            work->mode = DROP_FALL;
        }
        break;
    case DROP_LAND:
        /* like DROP_HOP's test, this never holds */
        if (work->flags & DROP_ALOFT) {
            sound_play_at(SE_DROP_BACK, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        work->pos.y = 0.0F;
        work->mode = DROP_CRAWL;
    case DROP_CRAWL:
        motion->speed = boss_work->pos;
        njSubVector(&motion->speed, &work->pos);
        motion->speed.y = 0.0F;
        distance = njSqrt(ACROSS_SQUARED(&motion->speed));
        motion->speed.x *= CRAWL_SPEED / distance;
        motion->speed.z *= CRAWL_SPEED / distance;
        njAddVector(&work->pos, &motion->speed);
        if (distance < BACK_NEAR) {
            work->mode = DROP_BACK;
            /* @hack redundant: a second break, which has DROP_BACK take R1 for
             * puddle_joined's 1 and R2 for the count (0x0c905102,
             * 0x0c905106); without it, 5 of 696 words differ */
            break;
        }
        break;
    case DROP_BACK:
        drops_left--;
        puddle_joined = 1;
        if (drops_left & 1) {
            sound_play_at(SE_DROP_BACK, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        work->mode = DROP_SHRINK;
    case DROP_SHRINK:
        njAddVector(&work->pos, &motion->speed);
        drop->size *= 0.97F;
        if (drop->size < 0.1F) {
            task_destroy(task);
            return;
        }
        break;
    }
    if (work->mode <= DROP_THROWN) {
        work->flags |= DROP_ALOFT;
    }
    drop->phase += NJM_DEG_ANG(20);
    drop->wobble *= 0.98F;
    if (work->mode >= DROP_LAND) {
        work->flags |= DROP_LANDED;
        work->collision->shapes[0].center.y = 0.0F;
    } else {
        work->collision->shapes[0].center.y = drop->size;
    }
    DROP(work)->frame += 8.0F / 30.0F;
    if (DROP(work)->frame >= 8.0F) {
        DROP(work)->frame -= 8.0F;
    }
    drop_draw(task);
    collision_register(work);
}

/* draw the core. once the drops are called back, it shows white while
 * chaos_record's steps is odd */
static void core_draw(struct task *task)
{
    struct task_work *work = task->work;

    draw_set_palette(4);
    draw_control_save();
    draw_control_clear(NJD_CONTROL_3D_CONSTANT_MATERIAL);
    draw_control_set(NJD_CONTROL_3D_OFFSET_MATERIAL);
    draw_attr_save();
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njSetTexture(&chaos2_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    /* @hack spelling: two tests, not one &&, which load draw_set_material's
     * address into R1 for the call (MOV.L at 0x0c905298); with &&, 12 of 76
     * words differ */
    if (drops_recalled) {
        if (chaos_record->steps & 1) {
            draw_set_material(0.0F, 1.0F, 1.0F, 1.0F);
        }
    }
    draw_tree(&chaos2_core_object);
    njPopMatrix(1);
    draw_control_restore();
    draw_attr_restore();
    draw_set_palette(0);
}

static void core_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct task_work *player = player_work[0];
    struct task *brain;

    switch (work->mode) {
    case CORE_START:
        core_work = work;
        work->pos.y = chaos_params->height * 0.5F;
        CORE(work)->brain.all = 0;
        work->mode = CORE_RISE;
        task->draw = core_draw;
    case CORE_RISE:
        work->pos.y += CORE_SPEED;
        if (work->pos.y >= CORE_LOW) {
            collision_init(task, core_shape, COLLISION_ITEM, 1, 1);
            work->mode = CORE_HOVER;
        }
        break;
    case CORE_HOVER:
        if (work->pos.y - player->pos.y < CORE_NEAR &&
            njDistanceP2P(&work->pos, &player->pos) < CORE_NEAR) {
            work->pos.y += CORE_CLIMB;
        } else {
            work->pos.y -= CORE_SPEED;
        }
        work->pos.y = CLAMP(work->pos.y, CORE_LOW, CORE_HIGH);
        if (drops_left < DROPS / 2) {
            work->mode = CORE_DROP;
            break;
        }
        if (work->flags & COLLISION_DAMAGED) {
            brain_shown = 0;
            drops_recalled = 1;
            work->mode = CORE_DROP;
        }
        break;
    case CORE_DROP:
        motion->speed.y = 6.0F * GRAVITY;
        work->mode = CORE_FALL;
    case CORE_FALL:
        motion->speed.y -= GRAVITY;
        work->pos.y += motion->speed.y;
        if (work->pos.y < -5.0F) {
            chaos_effect_splash(work, 16, 2.0F, 0.0F);
            puddle_joined = 1;
            task_destroy(task);
            return;
        }
        break;
    }
    if (work->mode <= CORE_HOVER) {
        CORE(work)->brain.steps++;
        if (CORE(work)->brain.steps > BRAIN_WAIT) {
            CORE(work)->brain.steps = 0;
            brain = task_create(TASK_WORK, 2, brain_once_step);
            brain->work->pos = work->pos;
            brain_shown = 1;
            brain->work->scl.x = 1.0F;
        }
    }
    core_draw(task);
    collision_register(work);
}

void chaos2_burst(struct task *task, struct task_work *work,
                  struct chaos *chaos, struct chaos *record)
{
    struct task *child;
    struct drop *drop;
    Angle heading;
    Float wave;
    Float size;
    int i;

    switch (work->submode) {
    case 0:
        brain_shown = 0;
        heading = chaos_angle_to_player(work, 0);
        for (i = 0; i < DROPS; i++) {
            child = task_create_child(TASK_MOTION | TASK_WORK, drop_step, task);
            child->extra = heap_calloc(1, sizeof(struct drop));
            drop = child->extra;
            drop->heading =
                heading + (Angle)((DROP_SPREAD -
                                   (rand() * (2 * DROP_SPREAD * RAND_SCALE))) *
                                  ANGLE_PER_DEGREE);
            child->work->pos = work->pos;
            child->work->pos.y += BALL_RADIUS;
            /* drop_step clears DROP_ALOFT before its switch, so setting it
             * here reaches neither drop_step's tests nor drop_draw's */
            if (i % 4 == 0) {
                child->work->flags |= DROP_ALOFT;
            }
        }
        drops_left = DROPS;
        child = task_create_child(TASK_MOTION | TASK_WORK, core_step, task);
        child->work->pos = work->pos;
        child =
            task_create_child(TASK_MOTION | TASK_WORK, chaos2_flash_step, task);
        child->work->pos = work->pos;
        child->work->pos.y += BALL_RADIUS;
        chaos->figures &= ~FIGURE_BALL;
        puddle_joined = 0;
        drops_recalled = 0;
        chaos2_set_shapes(work, SHAPES_NONE);
        chaos->flags |= CHAOS_NO_BEADS;
        blights_left = 5;
        blight_wait = 0;
        lights_left = 1;
        light_wait = 0;
        sound_play_at(SE_BURST, 0, 0, 0, work->pos.x, work->pos.y, work->pos.z);
        work->submode = 1;
    case 1:
        if (puddle_joined) {
            chaos->figures |= FIGURE_PUDDLE;
            if (burst_wobble < 0.05F) {
                burst_wobble = 0.15F;
            }
            puddle_joined = 0;
        }
        burst_phase += NJM_DEG_ANG(25);
        burst_wobble *= 0.92F;
        wave = njSin(burst_phase) * burst_wobble;
        /* the puddle's size is the share of the drops that are back */
        size = (DROPS - drops_left) * (1.0F / DROPS);
        size *= 1.0F + wave;
        chaos->scale.x = chaos->scale.z =
            size * chaos_params->full_scale * (1.0F + wave);
        chaos->scale.y = size * chaos_params->full_scale * (1.0F - wave);
        /* @hack spelling: two tests, not one &&, so the body takes
         * blights_left's address from its test (MOV.L at 0x0c9059a0); with &&,
         * the body loads it again and 99 of 546 words differ */
        if (--blight_wait < 0) {
            if (blights_left > 0) {
                blights_left--;
                blight_wait = BLIGHT_WAIT;
                child = chaos_create_child(TASK_MOTION | TASK_WORK,
                                           chaos2_blight_step, task, NULL);
                if (child != NULL) {
                    child->work->scl.x = child->work->scl.y =
                        child->work->scl.z = 1.0F;
                }
            }
        }
        /* @hack spelling: two tests, not one &&, so the body takes
         * lights_left's address from its test (MOV.L at 0x0c9059e2); with &&,
         * the body loads it again and 66 of 546 words differ */
        if (--light_wait < 0) {
            if (lights_left > 0) {
                lights_left--;
                light_wait = LIGHT_WAIT;
                chaos_create_child(TASK_MOTION | TASK_WORK, chaos2_light_step,
                                   task, NULL);
            }
        }
        if (drops_left <= 0) {
            if (work->mode != BOSS_FORM) {
                boss_next_mode = BOSS_FORM;
                boss_last_mode = work->mode;
            }
            boss_return_mode = BOSS_LEAP;
            work->submode = MODE_DONE;
        }
        break;
    /* @hack redundant: case 2, which no code sets, keeps the image's CMP/EQ #2
     * at 0x0c90554a; without it, 6 of 546 words differ */
    case 2:
        break;
    }
    if (work->submode == MODE_DONE) {
        task_destroy_children(task);
    }
}

void chaos2_shadow_draw(const NJS_POINT3 *pos, Float size)
{
    Float scale = size * (1.0F - (pos->y * 0.02F)) * 0.12F;

    if (scale > 0.0F) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_BOTHSRCALPHA);
        draw_set_material(0.8F, 1.0F, 1.0F, 1.0F);
        njPushMatrix(NULL);
        njTranslate(NULL, pos->x, 0.2F, pos->z);
        njRotateX(NULL, NJM_DEG_ANG(90));
        shadow_sprite.sx = scale;
        shadow_sprite.sy = scale;
        njDrawSprite3D(&shadow_sprite, 0, NJD_SPRITE_ALPHA);
        njPopMatrix(1);
    }
}

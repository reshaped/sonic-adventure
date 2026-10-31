/* the snow slides that chase player 0 down the board run once the board is
 * taken. the lower one comes to rest at a launch */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "effect/snow.h" /* NOLINT(misc-include-cleaner) */
#include "object/common.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg08/snow_slide.h"
#include "stg08/snowfall.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL upper_slide_model;
extern NJS_MODEL lower_slide_model;
/* drawn over the lower slide's model with h_nadd_texlist, whose eight
 * textures it steps through, one each second step */
extern NJS_MODEL lower_slide_cover_model;
extern NJS_TEXLIST h_nadd_texlist;
extern NJS_MODEL slide_clump_model;
extern NJS_MODEL slide_small_clump_model;

#define SE_SLIDE 223

/* once released, the upper slide runs down along its angles. below a height
 * of UPPER_TURN it turns and drops instead, stretching, until it passes
 * UPPER_FLOOR and stops. its task ends once player 0 is below UPPER_GONE */
#define UPPER_TURN (-250.0F)
#define UPPER_FLOOR (-500.0F)
#define UPPER_GONE (-1500.0F)
/* while more than UPPER_LEAD above player 0, the upper slide speeds up, and
 * otherwise it slows */
#define UPPER_LEAD 200.0F
/* the uvs of the upper slide's model, which scroll each step */
#define UPPER_UVS 224

/* the lower slide starts once released and player 0 is below LOWER_START.
 * it comes to rest at a height of LOWER_FLOOR. its task ends once player 0
 * is below LOWER_GONE */
#define LOWER_START (-700.0F)
#define LOWER_FLOOR (-3385.0F)
#define LOWER_GONE (-5000.0F)
/* while LOWER_FAR or farther from player 0, the lower slide speeds up without
 * limit, and otherwise it slows */
#define LOWER_FAR 300.0F
/* the lower slide's heading, and its launch's */
#define LOWER_HEADING (-0x4eee)
/* the way back up the lower slide's path, for a speed of 1. each step, the
 * slide moves by -speed times each. they are -sin(ang.y), sin(ang.z) and
 * -cos(ang.y) of its angles, to five places. the upper slide moves along its
 * angles the same way, working the terms out each step */
#define LOWER_X 0.93361F
#define LOWER_Y 0.52988F
#define LOWER_Z 0.35828F
/* in the lower slide's work->flags: it has come to rest at LOWER_FLOOR */
#define LOWER_RESTING 0x100
/* the speed a resting lower slide keeps for its powder and clumps */
#define REST_SPEED 5.0F
/* the lower slide's model holds LOWER_ROWS rows of LOWER_ROW_POINTS points.
 * every fourth step, a ripple moves each row's height by one of RIPPLES
 * offsets */
#define LOWER_ROWS 9
#define LOWER_ROW_POINTS 8
#define LOWER_POINTS (LOWER_ROWS * LOWER_ROW_POINTS)
#define RIPPLES 14
/* the textures of h_nadd_texlist, which the cover model steps through */
#define COVER_TEXTURES 8

/* each slide's timer as snow_slide_step sets it. each step, a slide's
 * textures scroll by an eighth of its timer */
#define UPPER_TIMER 96
#define LOWER_TIMER 128
/* at rest, the lower slide's models sink each step while its timer counts
 * down from LOWER_TIMER to SINK_END */
#define SINK_END 96

/* a slide shoves player 0 within its reach, UPPER_REACH or LOWER_REACH. the
 * shove sends player 0 flying at SHOVE_SPEED, its stick ignored for
 * SHOVE_STEPS. the slide then goes on at AFTER_SHOVE */
#define UPPER_REACH 240.0F
#define LOWER_REACH 205.0F
#define SHOVE_SPEED 7.5F
#define SHOVE_STEPS 10
#define AFTER_SHOVE 1.5F
/* the shove takes the slide's heading while player 0's bearing from the
 * slide lies within SHOVE_AHEAD of it. otherwise it takes a heading
 * SHOVE_ASIDE off the slide's */
#define SHOVE_AHEAD 0x600
#define SHOVE_ASIDE 0x1000

/* a launched player's stick is ignored for LAUNCH_STEPS */
#define LAUNCH_STEPS 60

/* the two slides' work->vars. the powder and the clumps read the lower
 * slide's speed */
struct slide_vars {
    Float stretch; /* the upper slide's scale along y */
    Angle roll; /* added to the upper slide's ang.z as it draws */
    Float speed;
};

#define SPRAY_IMAGES 8

struct spray_vars {
    Uint8 unk_00[4];
    /* the sprite shows image count / 2 of its SPRAY_IMAGES. count falls by 1
     * a step. the step after it reaches 0, it starts again at a random 12 to
     * 15 */
    Sint32 count;
    Sint32 slide; /* SPRAY_UPPER or SPRAY_LOWER */
};

/* the slide a spray belongs to. the spray keeps to its own bearing from the
 * slide, its ang.y. it stands 230 out from the upper slide, or 220 from the
 * lower */
#define SPRAY_UPPER 1
#define SPRAY_LOWER 2

struct powder_vars {
    Uint8 unk_00[4];
    Float scale;
};
/* the height above the lower slide at which powder is thrown again */
#define POWDER_TOP 25.0F

/* a clump is a lump of snow the lower slide throws up. kind 0 is a small
 * one, which starts out to either side. kinds 1 and 2 are big ones, which
 * start out each to its own side */
struct clump_vars {
    Uint8 unk_00[8];
    Sint32 kind;
};
/* the height above the lower slide at which a clump is thrown again */
#define CLUMP_TOP 40.0F
/* the shares of the slide's x and z move that a big and a small clump move
 * by */
#define BIG_CLUMP_FOLLOW 0.015625F
#define SMALL_CLUMP_FOLLOW 0.5F

#define VARS(work) ((struct slide_vars *)&(work)->vars)
#define SPRAY(work) ((struct spray_vars *)&(work)->vars)
#define POWDER(work) ((struct powder_vars *)&(work)->vars)
#define CLUMP(work) ((struct clump_vars *)&(work)->vars)
/* the lower slide's move this step, with y negated. the slide drops by y, and
 * the powder and the clumps rise by it */
#define SLIDE_MOVE(work) ((work)->scl)

static NJS_TEXANIM slide_spray_texanims[SPRAY_IMAGES] = {
#include "stg08/snow_slide/slide_spray_texanims.inc"
};
static NJS_SPRITE slide_spray_sprite = {
#include "stg08/snow_slide/slide_spray_sprite.inc"
};
static struct collision_shape lower_slide_shape[1] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     SHAPE_HURT | ATTACK(3),
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 150.0F, 100.0F},
     50.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};
static struct collision_shape slide_launch_shape[1] = {
    {SHAPE_RECTANGLE | SHAPE_HURTABLE,
     GUARD(3) | ATTACK(3),
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     150.0F,
     5.0F,
     80.0F,
     {0},
     0,
     NULL},
};
static Float lower_slide_ripples[RIPPLES] = {
#include "stg08/snow_slide/lower_slide_ripples.inc"
};
static const NJS_VECTOR slide_launch_speed = {15.0F, 0.0F, 0.0F};

static char snow_slide_released;
/* the y of each point of the lower slide's model, as snow_slide_step found
 * them. the ripples move the points from these */
static Float lower_slide_rest_heights[LOWER_POINTS];

static void upper_slide_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void lower_slide_destroy(struct task *task)
{
    /* @hack spelling: the model reached through its address, which loads it
     * into R6 and its points over it (0x0c905ea6, 0x0c905eae); from the model
     * itself, R3 takes it and 2 of 31 words differ */
    NJS_POINT3 *point = (&lower_slide_model)->points;
    int i;

    for (i = 0; i < LOWER_POINTS; ++i, ++point) {
        point->y = lower_slide_rest_heights[i];
    }
    task_destroy_children(task);
}

static void upper_slide_draw(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack spelling: ang reached through task->work, not work, which keeps
     * work in R13 and ang in R14 (0x0c905f28, 0x0c905f2c); as &work->ang,
     * the two trade and 9 of 63 words differ */
    struct angles *ang = &task->work->ang;
    Angle angle;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, ang->z + VARS(work)->roll, angle);
    TURN_VIA(njRotateX, ang->x + 640, angle);
    TURN_VIA(njRotateY, ang->y, angle);
    njScale(NULL, 1.0F, VARS(work)->stretch, 1.0F);
    draw_model(&upper_slide_model);
    njPopMatrix(1);
}

static void upper_slide_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    Float speed = VARS(work)->speed;
    /* @hack spelling: shove declared ahead of ang, which puts shove at sp+16
     * and ang at sp+4 (0x0c906262, 0x0c906198); the other way round, the
     * two trade and 5 of 306 words differ */
    NJS_VECTOR shove;
    Angle ang[3];
    NJS_COLOR *uv;
    int i;

    if (player->pos.y < UPPER_GONE) {
        task_destroy(task);
        njClipZ(-3.0F, -300.0F);
        return;
    }
    /* @hack spelling: the model reached through its address, which loads it and
     * then its meshsets at 12 (0x0c906040, 0x0c906048); from the model
     * itself, the pool holds the member's address and 2 of 306 words differ */
    uv = (&upper_slide_model)->meshsets->vertuv;
    for (i = 0; i < UPPER_UVS; ++i, ++uv) {
        uv->tex.v += TASK_TIMER(work) / 8;
    }
    if (work->pos.y < UPPER_TURN) {
        if (work->pos.y > UPPER_FLOOR) {
            work->ang.z += 64;
            work->ang.y += 20;
            work->pos.x += njSin(work->ang.y) * 2.0F;
            work->pos.y -= 4.0F;
            work->pos.z += njCos(work->ang.y) * 2.0F;
            VARS(work)->stretch += 0.05F;
        }
    } else if (snow_slide_released != 0) {
        if (work->pos.y > player->pos.y + UPPER_LEAD) {
            if (speed < 4.0F) {
                speed += 0.1F;
            }
        } else if (speed > 0.3F) {
            speed -= 0.29F;
        }
        if (njDistanceP2P(&work->pos, &player_work[0]->pos) < UPPER_REACH) {
            ang[0] = player->ang.x;
            ang[1] = (Angle)(atan2f(work->pos.x - player->pos.x,
                                    work->pos.z - player->pos.z) *
                             ANGLE_PER_RADIAN) -
                     work->ang.y - 0x8000;
            if (ang[1] < SHOVE_AHEAD && ang[1] > -SHOVE_AHEAD) {
                ang[1] = work->ang.y;
            } else if (ang[1] > 0) {
                ang[1] = work->ang.y - SHOVE_ASIDE;
            } else {
                ang[1] = work->ang.y + SHOVE_ASIDE;
            }
            ang[2] = player->ang.z;
            shove.x = SHOVE_SPEED;
            shove.y = 0.0F;
            shove.z = 0.0F;
            player_send_flying(0, &shove, ang, SHOVE_STEPS);
            speed = AFTER_SHOVE;
        }
        work->pos.x += njSin(work->ang.y) * speed;
        work->pos.y -= njSin(work->ang.z) * speed;
        work->pos.z += njCos(work->ang.y) * speed;
        VARS(work)->speed = speed;
    }
    sound_play_at_for(SE_SLIDE, (int)work, 1, 96, work->pos.x, work->pos.y,
                      work->pos.z, 2);
    upper_slide_draw(task);
    task_step_children(task);
}

static void lower_slide_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *pos = &work->pos;
    struct angles *ang = &work->ang;
    Angle angle;

    texture_set_stage();
    njPushMatrix(NULL);
    if (!(work->flags & LOWER_RESTING)) {
        njTranslateV(NULL, pos);
        TURN_VIA(njRotateZ, ang->z, angle);
        TURN_VIA(njRotateX, ang->x, angle);
        TURN_VIA(njRotateY, ang->y, angle);
        draw_model(&lower_slide_model);
        /* @hack redundant: a jump to the statement after the else, which puts
         * njPushMatrix's address in R1 at 0x0c906400; without it, R2 takes
         * it and 2 of 165 words differ */
        goto drawn;
    } else {
        njPushMatrix(NULL);
        njTranslateV(NULL, pos);
        TURN_VIA(njRotateZ, ang->z, angle);
        TURN_VIA(njRotateX, ang->x, angle);
        TURN_VIA(njRotateY, ang->y, angle);
        draw_model(&lower_slide_model);
        njPopMatrix(1);
        njTranslate(NULL, pos->x + (njSin(ang->y) * 15.0F),
                    pos->y + (njSin(ang->z) * 15.0F),
                    pos->z + (njCos(ang->y) * 15.0F));
        TURN_VIA(njRotateZ, ang->z, angle);
        TURN_VIA(njRotateX, ang->x, angle);
        TURN_VIA(njRotateY, ang->y, angle);
    }
drawn:
    /* @hack redundant: a never-run draw, which puts work in R11 and
     * draw_model's address in R12 (0x0c90634e, 0x0c906366); without it, the
     * two trade and 8 of 165 words differ */
    if (0) {
        draw_model(&lower_slide_cover_model);
    }
    njSetTexture(&h_nadd_texlist);
    draw_model(&lower_slide_cover_model);
    njPopMatrix(1);
}

static void lower_slide_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    Float speed = VARS(work)->speed;
    Float distance = njDistanceP2P(&player->pos, &work->pos);
    /* @hack spelling: shove declared ahead of ang, which puts shove at sp+20
     * and ang at sp+8 (0x0c906956, 0x0c9068d0); the other way round, the
     * two trade and 5 of 574 words differ */
    NJS_VECTOR shove;
    Angle ang[3];
    NJS_MODEL *model;
    NJS_MODEL *bound_model;
    NJS_POINT3 *point;
    NJS_COLOR *uv;
    Float ripple;
    int i;
    int j;

    if (player->pos.y < LOWER_GONE) {
        task_destroy(task);
        return;
    }
    model = &lower_slide_model;
    if (work->pos.y > LOWER_FLOOR) {
        if (distance >= LOWER_FAR) {
            speed += 0.18F;
        } else if (speed > 2.5F) {
            speed -= 0.4F;
        }
        if (!(work->mode & 3)) {
            work->tick = (work->tick + 1) % RIPPLES;
            point = model->points;
            /* @hack spelling: the increment written out, which keeps 2.0F
             * in FR6 and 3.0F in FR5 for the cover model's sinking below
             * (0x0c9066d4, 0x0c9066da); as ++i, the two trade and 14 of 574
             * words differ */
            for (i = 0; i < LOWER_ROWS; i = 1 + i) {
                ripple = lower_slide_ripples[(work->tick + (i * 2)) % RIPPLES];
                for (j = 0; j < LOWER_ROW_POINTS; ++j, ++point) {
                    point->y =
                        lower_slide_rest_heights[(i * LOWER_ROW_POINTS) + j] +
                        ripple;
                }
            }
        }
    } else {
        if (!(work->flags & LOWER_RESTING)) {
            /* LOWER_RESTING is clear here, so the add sets it */
            work->flags += LOWER_RESTING;
            work->pos.y = LOWER_FLOOR;
            point = model->points;
            for (i = 0; i < LOWER_POINTS; ++i, ++point) {
                point->y = lower_slide_rest_heights[i];
            }
            njClipZ(-3.0F, -1800.0F);
        }
        if (TASK_TIMER(work) > SINK_END) {
            --work->timer;
            /* @hack spelling: the model reached through its address, which
             * loads it into R4 and its points over it (0x0c9066c4, 0x0c9066d8);
             * from the model itself, R2 takes it and 2 of 574 words differ */
            point = (&lower_slide_cover_model)->points + 4;
            /* @hack spelling: the increment written out, under which SHC
             * unrolls the loop four times; as ++i, it doesn't, the function
             * is 98 bytes shorter and 522 of 574 words differ */
            for (i = 0; i < 11; i = 1 + i) {
                point->y -= 1.0F;
                ++point;
                point->y -= 2.0F;
                ++point;
                point->y -= 3.0F;
                ++point;
                point->y -= 4.0F;
                ++point;
                point->y -= 5.0F;
                /* point's last step, to index 103 of the cover model's 99
                 * points, would be undefined as pointer arithmetic */
                /* @hack type: the step taken on the address as an integer,
                 * which keeps 1.0F, 2.0F, 3.0F and 4.0F in FR4, FR6, FR5 and
                 * FR7 (0x0c9066ca to 0x0c9066da); as point += 5, they take
                 * other registers and 30 of 574 words differ */
                point =
                    (NJS_POINT3 *)((Uint32)point + (5 * sizeof(NJS_POINT3)));
            }
            point = model->points;
            for (i = 0; i < LOWER_ROW_POINTS; ++i, ++point) {
                point->y -= 5.0F;
            }
            for (i = 0; i < LOWER_ROW_POINTS; ++i, ++point) {
                point->y -= 4.0F;
            }
            /* @hack spelling: the increment written out, which keeps 2.0F in
             * FR6 and 3.0F in FR5 for the cover model's sinking above
             * (0x0c9066d4, 0x0c9066da); as ++i, the two trade and 14 of 574
             * words differ */
            for (i = 0; i < LOWER_ROW_POINTS; i = 1 + i, ++point) {
                point->y -= 3.0F;
            }
            /* @hack spelling: the increment written out, which keeps player
             * in R10 and the ripple's 14 in R8 (0x0c9064f4, 0x0c90656e), and
             * 2.0F in FR6 and 3.0F in FR5 for the cover model's sinking above;
             * as ++i, each pair trades and 25 of 574 words differ */
            for (i = 0; i < LOWER_ROW_POINTS; i = 1 + i, ++point) {
                point->y -= 2.0F;
            }
            for (i = 0; i < LOWER_ROW_POINTS; ++i, ++point) {
                point->y -= 1.0F;
            }
        }
        speed = 0.0F;
    }
    ++work->mode;
    if (!(work->mode & 1)) {
        /* @hack spelling: the model reached through its address, which reads
         * mats again through a copy of it (0x0c90687e) and loads it again
         * for the reset to 0 (0x0c906888); from the model itself, R4 keeps
         * it, the code after is 4 bytes early and 85 of 574 words differ */
        (&lower_slide_cover_model)->mats->attr_texId++;
        if ((&lower_slide_cover_model)->mats->attr_texId >= COVER_TEXTURES) {
            (&lower_slide_cover_model)->mats->attr_texId = 0;
        }
    }
    bound_model = model;
    /* @hack redundant: a never-run store, which keeps bound_model apart from
     * model: the walk's first test reads it through R6, a copy of R9 made at
     * 0x0c90688e; without it, the copy goes, the code after is 2 bytes early
     * and 71 of 574 words differ */
    if (0) {
        bound_model = 0;
    }
    uv = model->meshsets->vertuv;
    i = 0;
    if (bound_model->meshsets->nbMesh * 4 > 0) {
        do {
            uv->tex.v -= TASK_TIMER(work) / 8;
            /* @hack spelling: the increment written out, which keeps player
             * in R10 and the ripple's 14 in R8 (0x0c9064f4, 0x0c90656e); as
             * ++i, the two trade and 11 of 574 words differ */
            i = 1 + i;
            ++uv;
        } while (i < model->meshsets->nbMesh * 4);
    }
    if (distance < LOWER_REACH) {
        ang[0] = player->ang.x;
        ang[1] = (Angle)(atan2f(work->pos.x - player->pos.x,
                                work->pos.z - player->pos.z) *
                         ANGLE_PER_RADIAN) -
                 (LOWER_HEADING + 0x8000);
        if (ang[1] < SHOVE_AHEAD && ang[1] > -SHOVE_AHEAD) {
            ang[1] = LOWER_HEADING;
        } else if (ang[1] > 0) {
            ang[1] = LOWER_HEADING - SHOVE_ASIDE;
        } else {
            ang[1] = LOWER_HEADING + SHOVE_ASIDE;
        }
        ang[2] = player->ang.z;
        shove.x = SHOVE_SPEED;
        shove.y = 0.0F;
        shove.z = 0.0F;
        player_send_flying(0, &shove, ang, SHOVE_STEPS);
        speed = AFTER_SHOVE;
    }
    SLIDE_MOVE(work).x = -speed * LOWER_X;
    SLIDE_MOVE(work).y = speed * LOWER_Y;
    SLIDE_MOVE(work).z = -speed * LOWER_Z;
    work->pos.x += SLIDE_MOVE(work).x;
    work->pos.y -= SLIDE_MOVE(work).y;
    work->pos.z += SLIDE_MOVE(work).z;
    if (speed) {
        VARS(work)->speed = speed;
    } else {
        VARS(work)->speed = REST_SPEED;
    }
    sound_play_at_for(SE_SLIDE, (int)work, 1, 96, work->pos.x, work->pos.y,
                      work->pos.z, 2);
    lower_slide_draw(task);
    task_step_children(task);
    if (!placement_editing(task)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
}

static void slide_spray_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle angle;

    if (!frame_step) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, camera_work->ang.y, angle);
        TURN_VIA(njRotateX, camera_work->ang.x, angle);
        njDrawSprite3D(&slide_spray_sprite, SPRAY(work)->count / 2,
                       NJD_SPRITE_ALPHA);
        njPopMatrix(1);
    }
}

static void slide_spray_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *pos;
    NJS_POINT3 *center;
    Angle angle;

    if (!SPRAY(work)->count--) {
        SPRAY(work)->count =
            15 - (Uint8)_builtin_fabsf(rand() * (1.0F / 8192.0F));
    }
    center = &task->parent->work->pos;
    pos = &work->pos;
    angle = work->ang.y;
    if (SPRAY(work)->slide == SPRAY_UPPER) {
        pos->x = center->x + (njSin(angle) * 230.0F);
        pos->y = center->y + ((player_work[0]->pos.y - pos->y) * 0.5F) - 150.0F;
        pos->z = center->z + (njCos(angle) * 230.0F);
    } else {
        pos->x = center->x + (njSin(angle) * 220.0F);
        pos->y = center->y + ((player_work[0]->pos.y - pos->y) * 0.5F) - 50.0F;
        pos->z = center->z + (njCos(angle) * 220.0F);
    }
    /* @hack redundant: a discarded read of the flags, which keeps work in R12
     * and angle in R11 (0x0c906ade, 0x0c906b18); without it, the two trade
     * and 11 of 114 words differ */
    (void)work->flags;
    slide_spray_draw(task);
}

static void slide_powder_draw(struct task *task)
{
    Float scale = POWDER(task->work)->scale;
    Angle angle;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &task->work->pos);
    TURN_VIA(njRotateY, camera_work->ang.y, angle);
    njScale(NULL, scale, scale, scale);
    njDrawSprite3D(&snowfall_sprite, 0, NJD_SPRITE_ALPHA);
    njPopMatrix(1);
    /* @hack redundant: a return where the function ends, which makes the frame
     * with ADD #-4,R15 and stores task in it (0x0c906c24, 0x0c906c2a);
     * without it, a push does both, the code after is 2 bytes early and 45
     * of 80 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

/* throw the powder again from ahead of the lower slide. it flies up and out
 * to either side at twice the slide's speed */
static void slide_powder_reset(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;
    Float speed = VARS(parent)->speed;
    Angle pitch =
        (Angle)(rand() * (ANGLE_PER_DEGREE * 135.0F * RAND_SCALE)) - 0x3000;
    Angle heading = parent->ang.y;
    Float sine = njSin(pitch);
    Float cosine = njCos(pitch);
    NJS_POINT3 *pos;
    NJS_POINT3 *origin;

    work->scl.x = sine * njSin(heading + 0x4000) * speed * 2.0F;
    work->scl.y = cosine * speed * 2.0F;
    work->scl.z = sine * njCos(heading + 0x4000) * speed * 2.0F;
    pos = &work->pos;
    origin = &parent->pos;
    pos->x = origin->x + (work->scl.x * 25.0F) - (LOWER_X * 200.0F);
    pos->y = origin->y - (LOWER_Y * 200.0F);
    pos->z = origin->z + (work->scl.z * 25.0F) - (LOWER_Z * 200.0F);
}

/* the powder moves by its own speed plus SLIDE_MOVE. it shrinks as it nears
 * POWDER_TOP above the slide, where it is thrown again */
static void slide_powder_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;
    NJS_POINT3 *pos;
    NJS_VECTOR *drift;
    NJS_VECTOR *speed;

    if (work->pos.y > parent->pos.y + POWDER_TOP) {
        slide_powder_reset(task);
    } else {
        pos = &work->pos;
        drift = &SLIDE_MOVE(parent);
        speed = &work->scl;
        pos->x += speed->x + drift->x;
        pos->y += speed->y + drift->y;
        pos->z += speed->z + drift->z;
        POWDER(work)->scale = ((-pos->y + parent->pos.y) + POWDER_TOP) * 0.1F;
        if (POWDER(work)->scale > 4.0F) {
            POWDER(work)->scale = 4.0F;
        }
        slide_powder_draw(task);
    }
}

static void slide_clump_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle angle;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, angle);
    if (CLUMP(work)->kind) {
        draw_model(&slide_clump_model);
    } else {
        draw_model(&slide_small_clump_model);
    }
    njPopMatrix(1);
}

/* throw the clump again from ahead of the lower slide. its pitch splits the
 * slide's speed between its rise and its start's offset to the side */
static void slide_clump_reset(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;
    Float speed = VARS(parent)->speed;
    /* @hack type: kind as a Uint8, whose low byte the switch and the test
     * read (EXTU.B at 0x0c906f20); as the field's Sint32, 35 of 171 words
     * differ */
    Uint8 kind = CLUMP(work)->kind;
    Angle pitch;
    Float sine;
    Float cosine;
    Angle heading;
    NJS_POINT3 *pos;
    NJS_POINT3 *origin;

#ifdef GCC
    /* GCC can't see that every path sets pitch: the slide makes clumps of
     * kinds 0, 1 and 2 alone */
    pitch = 0;
#endif
    switch (kind) {
    case 0:
        pitch =
            (Angle)(rand() * (ANGLE_PER_DEGREE * 135.0F * RAND_SCALE)) - 0x3000;
        break;
    case 1:
        pitch =
            (Angle)(rand() * (ANGLE_PER_DEGREE * 65.0F * RAND_SCALE)) - 0x3000;
        break;
    case 2:
        pitch = (Angle)(((rand() * (65.0F * RAND_SCALE)) + 70.0F) *
                        ANGLE_PER_DEGREE) -
                0x3000;
        break;
    }
    sine = njSin(pitch);
    cosine = njCos(pitch);
    heading = parent->ang.y;
    work->scl.x = sine * njSin(heading + 0x4000) * speed;
    work->scl.y = cosine * speed;
    work->scl.z = sine * njCos(heading + 0x4000) * speed;
    pos = &work->pos;
    origin = &parent->pos;
    if (kind != 0) {
        pos->x = origin->x + (work->scl.x * 50.0F) - (LOWER_X * 230.0F);
        pos->z = origin->z + (work->scl.z * 50.0F) - (LOWER_Z * 230.0F);
    } else {
        pos->x = origin->x + (work->scl.x * 10.0F) - (LOWER_X * 200.0F);
        pos->z = origin->z + (work->scl.z * 10.0F) - (LOWER_Z * 200.0F);
    }
    pos->y = origin->y - (LOWER_Y * 225.0F);
    /* @bug arithmetic: the spin truncates rand() * RAND_SCALE, below
     * 1, to 0 (FTRC at 0x0c90705c) before it scales it by 1024 (0x0c907060),
     * so every clump spins 0xa00 a step */
    work->ang.y = ((Angle)(rand() * RAND_SCALE) * 1024) + 0xa00;
}

/* a clump moves by its share of the slide's x and z move. it rises at its
 * own speed plus the slide's drop. a clump more than CLUMP_TOP above the
 * slide is thrown again */
static void slide_clump_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;
    NJS_POINT3 *pos;
    NJS_VECTOR *drift;
    NJS_VECTOR *speed;

    if (work->pos.y > parent->pos.y + CLUMP_TOP) {
        slide_clump_reset(task);
    } else {
        pos = &work->pos;
        drift = &SLIDE_MOVE(parent);
        speed = &work->scl;
        /* @hack redundant: a discarded read of the parent's height, which holds
         * parent in R1 until a copy to R7 for drift (0x0c9070c2, 0x0c9070e2);
         * without it, parent takes R7 from the start, the code after is 2
         * bytes early and 45 of 59 words differ */
        (void)parent->pos.y;
        if (CLUMP(work)->kind) {
            pos->x += drift->x * BIG_CLUMP_FOLLOW;
            pos->z += drift->z * BIG_CLUMP_FOLLOW;
        } else {
            pos->x += drift->x * SMALL_CLUMP_FOLLOW;
            pos->z += drift->z * SMALL_CLUMP_FOLLOW;
        }
        pos->y += speed->y + drift->y;
        work->ang.z -= work->ang.y;
        slide_clump_draw(task);
    }
}

static void lower_slide_wait(struct task *task)
{
    if (snow_slide_released != 0 && player_work[0]->pos.y < LOWER_START) {
        task->step = lower_slide_step;
        task->draw = lower_slide_draw;
    }
}

static void slide_launch_step(struct task *task)
{
    struct task_work *work = task->work;
    int player;
    struct task_work *rider;
    Angle ang[3];
    NJS_VECTOR speed;

    if ((work->collision->flags & COLLISION_HIT) &&
        work->collision->hit->kind == COLLISION_PLAYER) {
        player = PLAYER_VARS(work->collision->hit->owner->work)->player;
        rider = player_work[player];
        if (rider->flags & PLAYER_ON_LAND) {
            speed = slide_launch_speed;
            ang[0] = rider->ang.x;
            ang[1] = LOWER_HEADING;
            ang[2] = rider->ang.z;
            player_send_flying(player, &speed, ang, LAUNCH_STEPS);
            njClipZ(-3.0F, -1800.0F);
        }
    }
    if (!placement_editing(task)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
}

void snow_slide_release(void)
{
    snow_slide_released = 1;
}

void snow_slide_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    NJS_POINT3 *point;
    int i;

    work->ang.x = -0x888;
    work->ang.y = LOWER_HEADING;
    work->ang.z = 0x16c1;
    work->pos.x = -469.0F;
    work->pos.y = -607.0F;
    work->pos.z = -266.0F;
    VARS(work)->speed = 2.5F;
    work->mode = 0;
    work->tick = 0;
    work->timer = LOWER_TIMER;
    /* @hack spelling: the clear written out, which keeps the first spray
     * loop's angle in R9 and its count in R10 (0x0c90737a, 0x0c907374); with
     * &=, the two trade and 28 of 389 words differ */
    work->flags = work->flags & ~LOWER_RESTING;
    /* @hack spelling: the first shape by its address, which starts the save
     * loop's index from R11's 0 through R1 (0x0c9072f2, 0x0c9072fa); as the
     * bare array, R4 copies R11 directly and 314 of 389 words differ */
    collision_init(task, &lower_slide_shape[0], COLLISION_ENEMY, 1, 1);
    work->collision->radius = 650.0F;
    /* @hack spelling: the model reached through its address, which loads it and
     * then its mats at 16 (0x0c9072f8, 0x0c9072fe); from the model itself,
     * the pool holds the member's address and 6 of 389 words differ */
    (&lower_slide_cover_model)->mats->attr_texId = 0;
    /* @hack spelling: the model reached through its address, which loads it
     * into R5 before the cover model's texture is set and its points after
     * (0x0c9072f0, 0x0c907302); from the model itself, both loads follow
     * that store and 5 of 389 words differ */
    point = (&lower_slide_model)->points;
    /* @hack spelling: the increment written out, which keeps the small clump
     * loop's count in R9 and each child in R10 (0x0c9074a0, 0x0c9074aa); as
     * ++i, the two trade and 11 of 389 words differ */
    for (i = 0; i < LOWER_POINTS; i = 1 + i, ++point) {
        lower_slide_rest_heights[i] = point->y;
    }
    child = task_create_child(TASK_WORK, slide_launch_step, task);
    work = child->work;
    work->ang.y = LOWER_HEADING;
    work->pos.x = -5350.0F;
    work->pos.y = -3325.0F;
    work->pos.z = -2100.0F;
    collision_init(child, slide_launch_shape, COLLISION_OBJECT, 1, 1);
    task->destructor = lower_slide_destroy;
    /* until the lower slide starts, its step and its draw are both the wait,
     * which draws nothing */
    task->step = lower_slide_wait;
    task->draw = lower_slide_wait;
    for (i = 0; i < 6; i++) {
        child = task_create_child(TASK_WORK, slide_spray_step, task);
        child->work->ang.y = LOWER_HEADING + (i * 0x700) - 0xe00;
        SPRAY(child->work)->count = i * 2;
        SPRAY(child->work)->slide = SPRAY_LOWER;
        child->draw = slide_spray_draw;
    }
    for (i = 0; i < 20; i++) {
        child = task_create_child(TASK_WORK, slide_powder_step, task);
        slide_powder_reset(child);
        child->draw = slide_powder_draw;
    }
    for (i = 0; i < 10; i++) {
        child = task_create_child(TASK_WORK, slide_clump_step, task);
        CLUMP(child->work)->kind = 0;
        slide_clump_reset(child);
        child->draw = slide_clump_draw;
    }
    child = task_create_child(TASK_WORK, slide_clump_step, task);
    CLUMP(child->work)->kind = 1;
    slide_clump_reset(child);
    child->draw = slide_clump_draw;
    child = task_create_child(TASK_WORK, slide_clump_step, task);
    CLUMP(child->work)->kind = 2;
    slide_clump_reset(child);
    child->draw = slide_clump_draw;

    task = task_create(TASK_WORK | TASK_MOTION, 3, upper_slide_step);
    work = task->work;
    work->pos.x = 341.0F;
    work->pos.y = 583.0F;
    work->pos.z = 45.0F;
    work->ang.x = 0xf480;
    work->ang.y = 0xb180;
    work->ang.z = 0x2e80;
    VARS(work)->stretch = 1.0F;
    VARS(work)->roll = -0x1400;
    VARS(work)->speed = 1.0F;
    work->timer = UPPER_TIMER;
    task->destructor = upper_slide_destroy;
    task->step = upper_slide_step;
    task->draw = upper_slide_draw;
    snow_slide_released = 0;
    for (i = 0; i < 6; i++) {
        child = task_create_child(TASK_WORK, slide_spray_step, task);
        child->work->ang.y = work->ang.y + (i * 0x700) - 0xd00;
        SPRAY(child->work)->count = i * 2;
        SPRAY(child->work)->slide = SPRAY_UPPER;
        child->draw = slide_spray_draw;
    }
}

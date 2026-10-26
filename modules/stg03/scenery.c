/* Twinkle Park's scenery. O FENCE 1, O FENCE 2 and O PLANT 1 to O PLANT 6 are
 * solid models. O BOW WINDOW and O GIRDER only draw. O LAMP is a lamp post
 * with two glows. O CANDLE 1 and O CANDLE 2 are candle stands with flames.
 * O FLAG 1, O FLAG 2 and O FLAG W LAMP are flags that wave near player 0 */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg03/effect_textures.h"
#include "stg03/scenery.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT twinkle_fence_1_object;
extern NJS_OBJECT twinkle_fence_2_object;
extern NJS_OBJECT twinkle_plant_1_object;
extern NJS_OBJECT twinkle_plant_2_object;
extern NJS_OBJECT twinkle_plant_3_object;
extern NJS_OBJECT twinkle_plant_4_object;
extern NJS_OBJECT twinkle_plant_5_object;
extern NJS_OBJECT twinkle_plant_6_object;
extern NJS_OBJECT twinkle_bow_window_object;
extern NJS_OBJECT twinkle_girder_object;
extern NJS_OBJECT twinkle_lamp_object;
extern NJS_OBJECT twinkle_candle_1_object;
extern NJS_OBJECT twinkle_candle_2_object;
extern NJS_OBJECT twinkle_flag_lamp_glow_object;
extern NJS_ACTION twinkle_flag_1_action;
extern NJS_ACTION twinkle_flag_2_action;
extern NJS_ACTION twinkle_flag_lamp_action;
extern NJS_MOTION twinkle_flag_1_shape;
extern NJS_MOTION twinkle_flag_2_shape;
extern NJS_MOTION twinkle_flag_lamp_shape;

/* O LAMP's and a flag's ranges for placement_try_leave_by, squared */
#define LAMP_RANGE (810.0F * 810.0F)
#define FLAG_RANGE (1510.0F * 1510.0F)

/* what each flag's step adds to FLAG_SPEED, and to FLAG_NEAR */
#define FLAG_WAVE 0.4F
#define FLAG_1_NEAR 300.0F
#define FLAG_2_NEAR 400.0F
#define FLAG_LAMP_NEAR 200.0F

/* a flame's frame count; the frames are candle01 to candle16 */
#define FLAME_FRAMES 16

#define QUARTER_TURN NJM_DEG_ANG(90)
#define HALF_TURN NJM_DEG_ANG(180)

enum scenery_mode { SCENERY_INIT, SCENERY_RUN };

struct candle_vars {
    NJS_OBJECT *object;
    /* the second and third flames' frames; the first's is in TASK_TIMER */
    Sint16 flame[2];
};

struct flag_vars {
    NJS_ACTION *action;
    Float frame;
    NJS_MOTION *shape;
};

#define CANDLE(work) ((struct candle_vars *)&(work)->vars)
/* the candle flames' sprite scale */
#define FLAME_SIZE(work) ((work)->scl.x)
#define FLAG(work) ((struct flag_vars *)&(work)->vars)
/* a fence's or plant's index in twinkle_scenery_kinds */
#define SCENERY_KIND(work) ((work)->tick)
/* the motion frames a flag advances each frame, and its distance from player
 * 0 within which it waves */
#define FLAG_SPEED(work) ((work)->scl.x)
#define FLAG_NEAR(work) ((work)->scl.y)

static struct collision_shape twinkle_scenery_shapes[5] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 23.7F, 0.0F},
     2.125F,
     23.7F,
     50.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 23.7F, -4.5F},
     3.625F,
     23.7F,
     4.5F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER2,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 4.0F, 0.0F},
     8.0F,
     4.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER2,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 3.0F, 0.0F},
     6.0F,
     3.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE2,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 4.0F, 0.0F},
     10.0F,
     4.0F,
     4.0F,
     {0},
     0,
     NULL},
};

struct scenery_kind {
    NJS_OBJECT *object;
    int shape; /* index in twinkle_scenery_shapes */
    Float reach; /* the reach of its collision record */
    Float range; /* its range for placement_try_leave_by, squared */
};

static struct scenery_kind twinkle_scenery_kinds[8] = {
    {&twinkle_fence_1_object, 0, 56.0F, 610.0F * 610.0F},
    {&twinkle_fence_2_object, 1, 50.0F, 610.0F * 610.0F},
    {&twinkle_plant_1_object, 2, 6.0F, 410.0F * 410.0F},
    {&twinkle_plant_2_object, 2, 6.0F, 410.0F * 410.0F},
    {&twinkle_plant_3_object, 3, 7.0F, 410.0F * 410.0F},
    {&twinkle_plant_4_object, 3, 7.0F, 410.0F * 410.0F},
    {&twinkle_plant_5_object, 4, 12.0F, 410.0F * 410.0F},
    {&twinkle_plant_6_object, 4, 12.0F, 410.0F * 410.0F},
};

static NJS_TEXANIM twinkle_candle_flames[FLAME_FRAMES] = {
#include "stg03/scenery/twinkle_candle_flames.inc"
};

static NJS_SPRITE twinkle_candle_sprite = {
#include "stg03/scenery/twinkle_candle_sprite.inc"
};

static void twinkle_scenery_destroy(struct task *task)
{
    task_kill(task);
}

static void twinkle_scenery_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(MODEL(work)->object);
        njPopMatrix(1);
    }
}

/* the step of a fence or plant, of the kind SCENERY_KIND gives */
static void twinkle_scenery_step(struct task *task)
{
    struct task_work *work = task->work;
    Uint8 kind = SCENERY_KIND(work);

    if (placement_try_leave_by(task, twinkle_scenery_kinds[kind].range)) {
        return;
    }
    if (edit_mode) {
        MODEL(work)->object = twinkle_scenery_kinds[kind].object;
        twinkle_scenery_draw(task);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "> NOT USE");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "> ROTATION");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "> ROTATION");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "> NOT USE");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "> NOT USE");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "> NOT USE");
        }
    } else {
        switch (work->mode) {
        case SCENERY_INIT:
            work->mode = SCENERY_RUN;
            work->ang.x = 0;
            work->scl.x = work->scl.y = work->scl.z = 0.0F;
            MODEL(work)->object = twinkle_scenery_kinds[kind].object;
            task->destructor = twinkle_scenery_destroy;
            task->draw = twinkle_scenery_draw;
            collision_init(
                task,
                &twinkle_scenery_shapes[twinkle_scenery_kinds[kind].shape],
                COLLISION_OBJECT, 1, 1);
            work->collision->radius = twinkle_scenery_kinds[kind].reach;
            break;
        case SCENERY_RUN:
            collision_register(work);
            twinkle_scenery_draw(task);
            break;
        }
    }
}

void twinkle_fence_1_step(struct task *task)
{
    struct task_work *work = task->work;

    SCENERY_KIND(work) = 0;
    twinkle_scenery_step(task);
}

void twinkle_fence_2_step(struct task *task)
{
    struct task_work *work = task->work;

    SCENERY_KIND(work) = 1;
    twinkle_scenery_step(task);
}

void twinkle_plant_1_step(struct task *task)
{
    struct task_work *work = task->work;

    SCENERY_KIND(work) = 2;
    twinkle_scenery_step(task);
}

void twinkle_plant_2_step(struct task *task)
{
    struct task_work *work = task->work;

    SCENERY_KIND(work) = 3;
    twinkle_scenery_step(task);
}

void twinkle_plant_3_step(struct task *task)
{
    struct task_work *work = task->work;

    SCENERY_KIND(work) = 4;
    twinkle_scenery_step(task);
}

void twinkle_plant_4_step(struct task *task)
{
    struct task_work *work = task->work;

    SCENERY_KIND(work) = 5;
    twinkle_scenery_step(task);
}

void twinkle_plant_5_step(struct task *task)
{
    struct task_work *work = task->work;

    SCENERY_KIND(work) = 6;
    twinkle_scenery_step(task);
}

void twinkle_plant_6_step(struct task *task)
{
    struct task_work *work = task->work;

    SCENERY_KIND(work) = 7;
    twinkle_scenery_step(task);
}

void twinkle_bow_window_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &twinkle_bow_window_object;
    model_start(task);
}

void twinkle_girder_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &twinkle_girder_object;
    model_start(task);
}

/* draw a glow at its object's offset, turned back from the lamp's heading to
 * face the camera */
static void twinkle_lamp_glow_draw(struct task_work *work, NJS_OBJECT *object)
{
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    TURN_VIA(njRotateY, -work->ang.y, ang);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    TURN_VIA(njRotateX, camera_work->ang.x, ang);
    draw_model(object->model);
    njPopMatrix(1);
}

/* on a frame's first step, draw the post, then its two glows: its first child
 * and that child's sibling */
static void twinkle_lamp_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = MODEL(work)->object;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njTranslateV(NULL, (NJS_VECTOR *)object->pos);
        draw_model(object->model);
        object = object->child;
        twinkle_lamp_glow_draw(work, object);
        twinkle_lamp_glow_draw(work, object->sibling);
        njPopMatrix(1);
    }
}

void twinkle_lamp_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    if (placement_try_leave_by(task, LAMP_RANGE)) {
        return;
    }
    object = &twinkle_lamp_object;
    if (edit_mode) {
        MODEL(work)->object = object;
        twinkle_lamp_draw(task);
    } else {
        switch (work->mode) {
        case SCENERY_INIT:
            work->mode = SCENERY_RUN;
            MODEL(work)->object = object;
            task->draw = twinkle_lamp_draw;
            task->destructor = twinkle_scenery_destroy;
            break;
        case SCENERY_RUN:
            twinkle_lamp_draw(task);
            break;
        }
    }
}

/* on a frame's first step, draw the stand's three flames at the offsets
 * first, second and third give. they blend additively, and each turns to the
 * camera's heading, the second a half turn further. the blend then goes back
 * to NJD_COLOR_BLENDING_BOTHSRCALPHA, and the constant material to opaque
 * white */
static void twinkle_candle_flames_draw(struct task *task, NJS_POINT3 *first,
                                       NJS_POINT3 *second, NJS_POINT3 *third)
{
    struct task_work *work = task->work;
    NJS_SPRITE *sprite = &twinkle_candle_sprite;
    NJS_ARGB material = {-1.0F, 1.0F, 1.0F, 1.0F};
    Angle facing = camera_work->ang.y;
    Angle ang;
    int first_frame = TASK_TIMER(work);
    int second_frame = CANDLE(work)->flame[0];
    int third_frame = CANDLE(work)->flame[1];

    if (frame_step == 0) {
        njSetConstantMaterial(&material);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njSetTexture(&eff_candle_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        sprite->sx = FLAME_SIZE(work);
        sprite->sy = FLAME_SIZE(work);
        njPushMatrix(NULL);
        njTranslateV(NULL, first);
        TURN(njRotateY, facing);
        njDrawSprite3D(sprite, first_frame,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_ANGLE);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslateV(NULL, second);
        TURN_VIA(njRotateY, facing + HALF_TURN, ang);
        njDrawSprite3D(sprite, second_frame,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_ANGLE);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslateV(NULL, third);
        TURN(njRotateY, facing);
        njDrawSprite3D(sprite, third_frame,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_ANGLE);
        njPopMatrix(1);
        njPopMatrix(1);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_BOTHSRCALPHA);
        material.a = 1.0F;
        njSetConstantMaterial(&material);
    }
}

/* draw one of the three candles the stand holds. it turns back from the
 * stand's heading to the camera's, less a quarter turn. each candle is half a
 * shell, so it shows the camera its round side */
static void twinkle_candle_part_draw(struct task_work *work, NJS_OBJECT *object)
{
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    TURN_VIA(njRotateY, -work->ang.y, ang);
    TURN_VIA(njRotateY, camera_work->ang.y - QUARTER_TURN, ang);
    draw_model(object->model);
    njPopMatrix(1);
}

/* draw the stand, then the three candles it holds. unlike the other draws
 * here, it draws on every step of a frame, not only the first */
static void twinkle_candle_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = CANDLE(work)->object;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    draw_model(object->model);
    object = object->child;
    twinkle_candle_part_draw(work, object);
    object = object->sibling;
    twinkle_candle_part_draw(work, object);
    twinkle_candle_part_draw(work, object->sibling);
    njPopMatrix(1);
}

/* draw the stand and its candles, with three flames that start at random
 * frames and loop. the task's draw leaves the flames out, so they vanish while
 * the game is paused */
void twinkle_candle_1_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (stage_get_stage_act() != STAGE_TWINKLE_MIRRORS) {
        return;
    }
    if (edit_mode) {
        CANDLE(work)->object = &twinkle_candle_1_object;
        twinkle_candle_draw(task);
    } else {
        /* the flames' offsets from the stand */
        NJS_POINT3 first = {3.5F, 16.0F, -6.0F};
        NJS_POINT3 second = {7.0F, 18.0F, 0.0F};
        NJS_POINT3 third = {3.5F, 16.0F, 6.0F};

        switch (work->mode) {
        case SCENERY_INIT:
            work->mode = SCENERY_RUN;
            FLAME_SIZE(work) = 0.4F;
            TASK_TIMER(work) = floorf(rand() * (FLAME_FRAMES * RAND_SCALE));
            CANDLE(work)->flame[0] =
                floorf(rand() * (FLAME_FRAMES * RAND_SCALE));
            CANDLE(work)->flame[1] =
                floorf(rand() * (FLAME_FRAMES * RAND_SCALE));
            CANDLE(work)->object = &twinkle_candle_1_object;
            task->draw = twinkle_candle_draw;
            task->destructor = twinkle_scenery_destroy;
            break;
        case SCENERY_RUN:
            if (++TASK_TIMER(work) >= FLAME_FRAMES) {
                TASK_TIMER(work) = 0;
            }
            if (++CANDLE(work)->flame[0] >= FLAME_FRAMES) {
                CANDLE(work)->flame[0] = 0;
            }
            if (++CANDLE(work)->flame[1] >= FLAME_FRAMES) {
                CANDLE(work)->flame[1] = 0;
            }
            twinkle_candle_draw(task);
            twinkle_candle_flames_draw(task, &first, &second, &third);
            break;
        }
    }
}

/* as twinkle_candle_1_step, with smaller flames */
void twinkle_candle_2_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (stage_get_stage_act() != STAGE_TWINKLE_MIRRORS) {
        return;
    }
    if (edit_mode) {
        CANDLE(work)->object = &twinkle_candle_2_object;
        twinkle_candle_draw(task);
    } else {
        /* the flames' offsets, lower and closer together than O CANDLE 1's */
        NJS_POINT3 first = {3.0F, 11.5F, -5.0F};
        NJS_POINT3 second = {5.0F, 13.0F, 0.0F};
        NJS_POINT3 third = {3.0F, 11.5F, 5.0F};

        switch (work->mode) {
        case SCENERY_INIT:
            work->mode = SCENERY_RUN;
            FLAME_SIZE(work) = 0.3F;
            TASK_TIMER(work) = floorf(rand() * (FLAME_FRAMES * RAND_SCALE));
            CANDLE(work)->flame[0] =
                floorf(rand() * (FLAME_FRAMES * RAND_SCALE));
            CANDLE(work)->flame[1] =
                floorf(rand() * (FLAME_FRAMES * RAND_SCALE));
            CANDLE(work)->object = &twinkle_candle_2_object;
            task->draw = twinkle_candle_draw;
            task->destructor = twinkle_scenery_destroy;
            break;
        case SCENERY_RUN:
            if (++TASK_TIMER(work) >= FLAME_FRAMES) {
                TASK_TIMER(work) = 0;
            }
            if (++CANDLE(work)->flame[0] >= FLAME_FRAMES) {
                CANDLE(work)->flame[0] = 0;
            }
            if (++CANDLE(work)->flame[1] >= FLAME_FRAMES) {
                CANDLE(work)->flame[1] = 0;
            }
            twinkle_candle_draw(task);
            twinkle_candle_flames_draw(task, &first, &second, &third);
            break;
        }
    }
}

/* on a frame's first step, draw the flag. within FLAG_NEAR of player 0, it
 * waves by its motion and shape motion. farther off, it hangs still. near or
 * far, each draw outside the set editor advances its frame by FLAG_SPEED */
static void twinkle_flag_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        texture_set_stage();
        if (njDistanceP2P(&work->pos, &player_work[0]->pos) < FLAG_NEAR(work)) {
            draw_shape_motion(FLAG(work)->action->object,
                              FLAG(work)->action->motion, FLAG(work)->shape,
                              FLAG(work)->frame);
        } else {
            draw_tree(FLAG(work)->action->object);
        }
        njPopMatrix(1);
        if (!edit_mode) {
            if ((FLAG(work)->frame += FLAG_SPEED(work)) >=
                FLAG(work)->action->motion->nbFrame) {
                FLAG(work)->frame = 0.0F;
            }
        }
    }
}

void twinkle_flag_1_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ACTION *action;
    NJS_MOTION *shape;

    if (placement_try_leave_by(task, FLAG_RANGE)) {
        return;
    }
    shape = &twinkle_flag_1_shape;
    action = &twinkle_flag_1_action;
    if (edit_mode) {
        FLAG(work)->action = action;
        FLAG(work)->shape = shape;
        twinkle_flag_draw(task);
    } else {
        switch (work->mode) {
        case SCENERY_INIT:
            work->mode = SCENERY_RUN;
            FLAG_SPEED(work) += FLAG_WAVE;
            FLAG_NEAR(work) += FLAG_1_NEAR;
            FLAG(work)->action = action;
            FLAG(work)->shape = shape;
            task->draw = twinkle_flag_draw;
            task->destructor = twinkle_scenery_destroy;
            break;
        case SCENERY_RUN:
            twinkle_flag_draw(task);
            break;
        }
    }
}

void twinkle_flag_2_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ACTION *action;
    NJS_MOTION *shape;

    if (placement_try_leave_by(task, FLAG_RANGE)) {
        return;
    }
    shape = &twinkle_flag_2_shape;
    action = &twinkle_flag_2_action;
    if (edit_mode) {
        FLAG(work)->action = action;
        FLAG(work)->shape = shape;
        twinkle_flag_draw(task);
    } else {
        switch (work->mode) {
        case SCENERY_INIT:
            work->mode = SCENERY_RUN;
            FLAG_SPEED(work) += FLAG_WAVE;
            FLAG_NEAR(work) += FLAG_2_NEAR;
            FLAG(work)->action = action;
            FLAG(work)->shape = shape;
            task->draw = twinkle_flag_draw;
            task->destructor = twinkle_scenery_destroy;
            break;
        case SCENERY_RUN:
            twinkle_flag_draw(task);
            break;
        }
    }
}

/* draw the flag as twinkle_flag_draw does, under three glows. each glow turns
 * back from the flag's heading to face the camera */
static void twinkle_flag_lamp_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *glow = &twinkle_flag_lamp_glow_object;
    Angle ang;

    if (frame_step == 0) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        texture_set_stage();
        if (njDistanceP2P(&work->pos, &player_work[0]->pos) < FLAG_NEAR(work)) {
            draw_shape_motion(FLAG(work)->action->object,
                              FLAG(work)->action->motion, FLAG(work)->shape,
                              FLAG(work)->frame);
        } else {
            draw_tree(FLAG(work)->action->object);
        }
        njPushMatrix(NULL);
        njTranslate(NULL, -5.97F, 54.0F, 3.5F);
        TURN_VIA(njRotateY, -work->ang.y, ang);
        TURN_VIA(njRotateY, camera_work->ang.y, ang);
        TURN_VIA(njRotateX, camera_work->ang.x, ang);
        draw_tree(glow);
        njPopMatrix(1);
        njPushMatrix(NULL);
        /* @hack spelling: x as -0.0F, which the image's pool holds at
         * 0x0c900e28 for FMOV.S to load (0x0c900ce6); as 0.0F, 125 of 229
         * words differ */
        njTranslate(NULL, -0.0F, 54.0F, -7.0F);
        TURN_VIA(njRotateY, -work->ang.y, ang);
        TURN_VIA(njRotateY, camera_work->ang.y, ang);
        TURN_VIA(njRotateX, camera_work->ang.x, ang);
        draw_tree(glow);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, 5.97F, 54.0F, 3.5F);
        TURN_VIA(njRotateY, -work->ang.y, ang);
        TURN_VIA(njRotateY, camera_work->ang.y, ang);
        TURN_VIA(njRotateX, camera_work->ang.x, ang);
        draw_tree(glow);
        njPopMatrix(1);
        njPopMatrix(1);
        if (!edit_mode) {
            if ((FLAG(work)->frame += FLAG_SPEED(work)) >=
                FLAG(work)->action->motion->nbFrame) {
                FLAG(work)->frame = 0.0F;
            }
        }
    }
}

void twinkle_flag_lamp_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ACTION *action;
    NJS_MOTION *shape;

    if (placement_try_leave_by(task, FLAG_RANGE)) {
        return;
    }
    shape = &twinkle_flag_lamp_shape;
    action = &twinkle_flag_lamp_action;
    if (edit_mode) {
        FLAG(work)->action = action;
        FLAG(work)->shape = shape;
        twinkle_flag_lamp_draw(task);
    } else {
        switch (work->mode) {
        case SCENERY_INIT:
            work->mode = SCENERY_RUN;
            FLAG_SPEED(work) += FLAG_WAVE;
            FLAG_NEAR(work) += FLAG_LAMP_NEAR;
            FLAG(work)->action = action;
            FLAG(work)->shape = shape;
            task->draw = twinkle_flag_lamp_draw;
            task->destructor = twinkle_scenery_destroy;
            break;
        case SCENERY_RUN:
            twinkle_flag_lamp_draw(task);
            break;
        }
    }
}

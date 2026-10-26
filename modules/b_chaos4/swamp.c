/* Chaos 4's swamp. its waves are a model whose points the ripples move,
 * and the water, a thick liquid in the world, rides the same model. the
 * water draws added to what is behind it. a calm surface, whose texture
 * cycles through chaos4_numa_textures, shows before the fight and after it.
 * the two fade into each other. below both lies the solid bottom, and
 * player 0 falls out of the stage below y 0 */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos4/camera.h"
#include "b_chaos4/numa_textures.h"
#include "b_chaos4/swamp.h"
#include "b_chaos4/textures.h"
#include "b_chaos4/util.h"
#include "collision/world.h"
#include "draw/tree.h"
#include "enemy/enemy.h"
#include "hud/life_gauge.h"
#include "player/player.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

/* calm_texlist holds one texture, which calm_create and calm_step point
 * at an entry of chaos4_numa_textures */
extern NJS_TEXLIST calm_texlist;
extern NJS_OBJECT waves_object;
extern NJS_OBJECT calm_object;
extern NJS_OBJECT swamp_bottom_object;

/* the world flags of the waves, of the water WATER_ABOVE over them, and of
 * the bottom. the waves take no surface flag */
#define WAVES_FLAGS (WORLD_SMALL | WORLD_MOVES)
#define WATER_FLAGS (WORLD_MOVES | WORLD_THICK_LIQUID)
#define BOTTOM_FLAGS (WORLD_LARGE | WORLD_DEFLECT | WORLD_SOLID)
#define WATER_ABOVE 0.1F

/* swamp_create sets the water's material to this color. swamp_draw sets
 * red, green and blue to 0 before each use, so the color never draws */
#define WATER_RED 1.0F
#define WATER_GREEN 0.474F
#define WATER_BLUE 0.0F

/* while player 0 stands on any task's object and the boss has health, the
 * waves sink by WAVES_SINK a step. meanwhile, while player 0 is at or below
 * SHAKE_DEPTH, pad 0's vibration pack shakes at SHAKE_POWER and SHAKE_FREQ.
 * otherwise the waves stand at WAVES_REST */
#define WAVES_SINK (-0.04F)
#define SHAKE_DEPTH 11.0F
#define WAVES_REST (-5.0F)
#define SHAKE_POWER 3
#define SHAKE_FREQ 30

/* each frame, a ripple pushes up each point within RIPPLE_REACH of its
 * ring in x and z. the push grows with the point's distance from the ring,
 * from 0 at the ring itself, and with the ripple's strength. the ring
 * spreads by RIPPLE_SPREAD a frame, and its strength falls from 1 to 0 in
 * 200 frames */
#define RIPPLE_REACH 50.0F
#define RIPPLE_PUSH 0.1F
#define RIPPLE_SPREAD 0.9F
#define RIPPLE_FADE (0.3F / 60)
/* each frame, a point's speed keeps WAVE_DRAG of itself, and loses
 * WAVE_SPRING of the point's offset, which springs it back to its rest */
#define WAVE_DRAG 0.99F
#define WAVE_SPRING 0.005F

/* each step, a splash lands on the swamp's surface at a random place within
 * SPLASH_HALF of SPLASH_X and SPLASH_Z */
#define SPLASH_X (-107.0F)
#define SPLASH_Z (-285.0F)
#define SPLASH_HALF 17.5F

/* a fade moves calm_alpha and waves_alpha in two halves, each counting
 * steps down from FADE_TIME past 0. in the first half, the alpha that falls
 * loses FADE_EARLY a step and the alpha that rises gains FADE_LATE. the
 * second half swaps the two */
#define FADE_TIME 15
#define FADE_EARLY (0.3F / 15)
#define FADE_LATE (0.7F / 15)

struct swamp_vars {
    Uint8 unk_00[8];
    Float unk_08; /* swamp_create clears it, and nothing in swamp.c reads it */
};

#define SWAMP_VARS(work) ((struct swamp_vars *)&(work)->vars)

struct fade_vars {
    Sint32 to_waves;
    Sint32 timer;
};

#define FADE_VARS(work) ((struct fade_vars *)&(work)->vars)

enum fade_mode {
    FADE_START,
    WAVES_IN,
    WAVES_IN_LATE,
    CALM_IN,
    CALM_IN_LATE,
    FADE_END
};

static void swamp_world_init(struct swamp *swamp, struct task *task);
static void swamp_step(struct task *task);
static void swamp_waves_step(struct swamp *swamp);
static void swamp_ripples_step(struct swamp *swamp);
static void swamp_splash(void);
static void swamp_draw(struct task *task);
static void swamp_destroy(struct task *task);
static void calm_create(void);
static void calm_step(struct task *task);
static void calm_draw(struct task *task);
static void fade_step(struct task *task);
static void bottom_init(struct task *task);
static void bottom_step(struct task *task);
static void bottom_destroy(struct task *task);

/* the constant material alphas of the calm surface and of the water. at 0
 * each shows whole, and at -1 or less its draw skips it */
static Float calm_alpha = 0.0F;
static Float waves_alpha = 0.0F;

struct swamp_ripple *swamp_ripples;
struct swamp *swamp_current;
/* no code reads or writes this word. it keeps calm_frame at 0x0c95e554 */
static Sint32 dat_0c95e550;
/* the index in chaos4_numa_textures of the calm surface's next texture */
static Uint32 calm_frame;

void swamp_create(void)
{
    struct task *task;
    struct task_work *work;
    struct swamp *swamp;
    NJS_POINT3 *point;
    int i;

    swamp_current = NULL;
    swamp_ripples = NULL;
    if ((task = task_create(TASK_WORK | TASK_FORCE, 3, swamp_step)) != NULL) {
        work = task->work;
        enemy_alloc_record(task, work);
        /* @bug memory: this return leaves the task running swamp_step with no
         * extra, and the step reads swamp->waves through the NULL (BRA at
         * 0x0c906f08 to the return; MOV.L @R14,R4 at 0x0c907256, R14 the
         * extra) */
        if ((swamp = heap_calloc(1, sizeof(struct swamp))) == NULL) {
            return;
        }
        task->extra = swamp;
        swamp->speeds = heap_calloc(waves_object.model->nbPoint, sizeof(Float));
        swamp->offsets =
            heap_calloc(waves_object.model->nbPoint, sizeof(Float));
        swamp->rests = heap_calloc(waves_object.model->nbPoint, sizeof(Float));
        if (!swamp->speeds || !swamp->offsets || !swamp->rests) {
            if (swamp->speeds) {
                heap_free(swamp->speeds);
            }
            if (swamp->offsets) {
                heap_free(swamp->offsets);
            }
            if (swamp->rests) {
                heap_free(swamp->rests);
            }
            task_kill(task);
            return;
        }
        swamp_world_init(swamp, task);
        for (i = 0; i < waves_object.model->nbPoint; i++) {
            point = &waves_object.model->points[i];
            swamp->rests[i] = point->y;
        }
        swamp_ripples = swamp->ripples;
        swamp_current = swamp;
        swamp->material.a = waves_alpha;
        swamp->material.r = WATER_RED;
        swamp->material.g = WATER_GREEN;
        swamp->material.b = WATER_BLUE;
        SWAMP_VARS(work)->unk_08 = 0.0F;
        task->step = swamp_step;
        task->draw = swamp_draw;
        task->destructor = swamp_destroy;
    }
    calm_create();
    calm_alpha = 0.0F;
    waves_alpha = -1.0F;
}

/* put the waves in the world as a copy of waves_object with a model and
 * points of its own. the water goes in as another copy, on the waves'
 * model */
static void swamp_world_init(struct swamp *swamp, struct task *task)
{
    NJS_OBJECT *object;
    NJS_OBJECT *water;
    NJS_MODEL *model;
    int i;

    swamp->waves = object = world_object_alloc();
    *object = waves_object;
    object->model = heap_calloc(1, sizeof(NJS_MODEL));
    object->evalflags &= ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
    model = object->model;
    *model = *waves_object.model;
    model->points =
        heap_calloc(waves_object.model->nbPoint, sizeof(NJS_POINT3));
    for (i = 0; i < waves_object.model->nbPoint; i++) {
        VECTOR_COPY(&model->points[i], &waves_object.model->points[i]);
    }
    task->work->flags |= WORLD_TESTED;
    world_add(WAVES_FLAGS, task, object);
    water = world_object_alloc();
    *water = waves_object;
    water->model = object->model;
    water->pos[1] += WATER_ABOVE;
    swamp->water = water;
    world_add(WATER_FLAGS, task, water);
}

/* move the waves, leave a splash, and draw the swamp. the points and the
 * ripples move on a frame's first step only, and never for Knuckles. the y
 * of the task's first force, player 0's, takes the waves' move this step */
static void swamp_step(struct task *task)
{
    struct swamp *swamp = task->extra;
    NJS_MODEL *model = swamp->waves->model;
    Float y;

    /* @hack redundant: a never-run free of model, which keeps the load of model
     * the image has, which nothing reads (MOV.L @(4,R4),R4 at 0x0c907258);
     * without it, 76 of 86 words differ */
    if (0) {
        heap_free(model);
    }
    task->work->flags |= WORLD_TESTED;
    if (frame_step == 0 && get_character() != CHARACTER_KNUCKLES) {
        swamp_waves_step(swamp);
        swamp_ripples_step(swamp);
    }
    y = swamp->waves->pos[1];
    if (player_get(0)->ground_task && life_gauge_health > 0.0F) {
        swamp->waves->pos[1] += WAVES_SINK;
        if (player_work[0]->pos.y <= SHAKE_DEPTH) {
            vibration_start(0, SHAKE_POWER, SHAKE_FREQ, 1);
        }
    } else {
        swamp->waves->pos[1] = WAVES_REST;
    }
    ((struct task_force *)task->force)->speed.y = swamp->waves->pos[1] - y;
    swamp_splash();
    swamp_draw(task);
}

/* move each point of the waves' model by the ripples near it and by its
 * spring */
static void swamp_waves_step(struct swamp *swamp)
{
    NJS_MODEL *model = swamp->waves->model;
    NJS_POINT3 *point;
    Float *speed;
    Float *offset;
    Float *rest;
    struct swamp_ripple *ripple;
    NJS_VECTOR to;
    Float distance;
    int i;
    int j;

    for (i = 0; i < model->nbPoint; i++) {
        point = &model->points[i];
        speed = &swamp->speeds[i];
        offset = &swamp->offsets[i];
        rest = &swamp->rests[i];
        for (j = 0; j < SWAMP_RIPPLES; j++) {
            ripple = &swamp->ripples[j];
            if (ripple->strength > 0.0F) {
                VECTOR_DIFF(&to, point, &ripple->pos);
                distance = _builtin_fabsf(njSqrt(SQUARE(to.x) + SQUARE(to.z)) -
                                          ripple->radius);
                if (distance < RIPPLE_REACH) {
                    /* one product of the constants, 0x3a03126d, as SHC
                     * folds them */
                    *speed += distance * ripple->strength *
                              (RIPPLE_PUSH * WAVE_SPRING);
                }
            }
        }
        *speed *= WAVE_DRAG;
        *speed -= *offset * WAVE_SPRING;
        *offset += *speed;
        point->y = *rest + *offset;
    }
}

static void swamp_ripples_step(struct swamp *swamp)
{
    struct swamp_ripple *ripple;
    int j;

    for (j = 0; j < SWAMP_RIPPLES; j++) {
        ripple = &swamp->ripples[j];
        if (ripple->strength > 0.0F) {
            ripple->radius += RIPPLE_SPREAD;
            ripple->strength -= RIPPLE_FADE;
        }
    }
}

static void swamp_splash(void)
{
    NJS_POINT3 pos;

    pos.x = SPLASH_X;
    pos.z = SPLASH_Z;
    pos.y = SWAMP_Y;
    pos.x += RANDOM(-SPLASH_HALF, SPLASH_HALF);
    pos.z += RANDOM(-SPLASH_HALF, SPLASH_HALF);
    chaos4_splash_create(&pos);
}

/* on a frame's first step, draw the dome, and the water while waves_alpha
 * is above -1. the water draws added to what is behind it */
static void swamp_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct swamp *swamp = task->extra;

    if (frame_step == 0) {
        chaos4_dome_draw();
        if (waves_alpha > -1.0F) {
            swamp->material.a = waves_alpha;
            swamp->material.r = 0.0F;
            swamp->material.g = 0.0F;
            swamp->material.b = 0.0F;
            njSetConstantMaterial(&swamp->material);
            njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
            njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
            njSetTexture(&chaos4_object_texlist);
            njPushMatrix(NULL);
            njTranslateV(NULL, &work->pos);
            draw_tree(swamp->water);
            njPopMatrix(1);
        }
    }
}

/* the waves' object goes back to the pool before the destructor reads its
 * model. world_object_free only marks the object free, and leaves its
 * model in place */
static void swamp_destroy(struct task *task)
{
    struct swamp *swamp = task->extra;

    world_object_free(swamp->waves);
    world_object_free(swamp->water);
    heap_free(swamp->rests);
    heap_free(swamp->offsets);
    heap_free(swamp->speeds);
    heap_free(swamp->waves->model->points);
    heap_free(swamp->waves->model);
    swamp_ripples = NULL;
    swamp_current = NULL;
}

void swamp_ripple_add(NJS_POINT3 *pos)
{
    struct swamp_ripple *ripple;
    int i;

    if (swamp_ripples) {
        for (i = 0; i < SWAMP_RIPPLES; i++) {
            ripple = &swamp_ripples[i];
            if (!(ripple->strength > 0.0F)) {
                VECTOR_COPY(&ripple->pos, pos);
                ripple->strength = 1.0F;
                ripple->radius = 0.0F;
                break;
            }
        }
    }
}

/* start the calm surface's task, on the first of chaos4_numa_textures */
static void calm_create(void)
{
    struct task *task;

    if ((task = task_create(TASK_WORK, 3, calm_step)) != NULL) {
        enemy_alloc_record(task, task->work);
        calm_frame = 0;
        calm_texlist.textures = chaos4_numa_textures;
        task->step = calm_step;
        task->draw = calm_draw;
    }
}

static void calm_step(struct task *task)
{
    calm_draw(task);
    if (frame_step != 0) {
        return;
    }
    calm_texlist.textures = &chaos4_numa_textures[calm_frame];
    if (++calm_frame >= chaos4_numa_texlist.nbTexture) {
        calm_frame = 0;
    }
}

static void calm_draw(struct task *task)
{
    NJS_ARGB color;

    if (calm_alpha > -1.0F && frame_step == 0) {
        njSetTexture(&calm_texlist);
        color.a = calm_alpha;
        color.r = color.g = color.b = 0.0F;
        njSetConstantMaterial(&color);
        njPushMatrix(NULL);
        draw_tree(&calm_object);
        njPopMatrix(1);
    }
}

void swamp_fade(int to_waves)
{
    struct task *task;
    struct task_work *work;

    if ((task = task_create(TASK_WORK, 3, fade_step)) != NULL) {
        work = task->work;
        enemy_alloc_record(task, work);
        /* FADE_START sets the timer before anything reads it, so this 30 is
         * never read */
        FADE_VARS(work)->timer = 30;
        FADE_VARS(work)->to_waves = to_waves;
        task->step = fade_step;
    }
}

/* FADE_START shows the side the fade leaves and hides the other. after
 * the two halves, FADE_END sets both alphas to the fade's end exactly and
 * ends the fade's task */
static void fade_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case FADE_START:
        FADE_VARS(work)->timer = FADE_TIME;
        if (FADE_VARS(work)->to_waves) {
            calm_alpha = 0.0F;
            waves_alpha = -1.0F;
            work->mode = WAVES_IN;
        } else {
            calm_alpha = -1.0F;
            waves_alpha = 0.0F;
            work->mode = CALM_IN;
        }
        break;
    case WAVES_IN:
        calm_alpha -= FADE_EARLY;
        waves_alpha += FADE_LATE;
        if (--FADE_VARS(work)->timer < 0) {
            FADE_VARS(work)->timer = FADE_TIME;
            work->mode = WAVES_IN_LATE;
        }
        break;
    case WAVES_IN_LATE:
        calm_alpha -= FADE_LATE;
        waves_alpha += FADE_EARLY;
        if (--FADE_VARS(work)->timer < 0) {
            FADE_VARS(work)->timer = FADE_TIME;
            work->mode = FADE_END;
        }
        break;
    case CALM_IN:
        waves_alpha -= FADE_EARLY;
        calm_alpha += FADE_LATE;
        if (--FADE_VARS(work)->timer < 0) {
            FADE_VARS(work)->timer = FADE_TIME;
            work->mode = CALM_IN_LATE;
        }
        break;
    case CALM_IN_LATE:
        waves_alpha -= FADE_LATE;
        calm_alpha += FADE_EARLY;
        if (--FADE_VARS(work)->timer < 0) {
            FADE_VARS(work)->timer = FADE_TIME;
            work->mode = FADE_END;
        }
        break;
    case FADE_END:
        if (FADE_VARS(work)->to_waves) {
            calm_alpha = -1.0F;
            waves_alpha = 0.0F;
        } else {
            calm_alpha = 0.0F;
            waves_alpha = -1.0F;
        }
        task_kill(task);
        break;
    }
}

void swamp_bottom_create(void)
{
    struct task *task;

    if ((task = task_create(TASK_WORK | TASK_FORCE, 3, bottom_step)) != NULL) {
        enemy_alloc_record(task, task->work);
        bottom_init(task);
        task->step = bottom_step;
        task->destructor = bottom_destroy;
    }
}

static void bottom_init(struct task *task)
{
    NJS_OBJECT *object = world_object_alloc();

    *object = swamp_bottom_object;
    object->evalflags &= ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
    task->extra = object;
    task->work->flags |= WORLD_TESTED;
    world_add(BOTTOM_FLAGS, task, object);
}

static void bottom_step(struct task *task)
{
    if (player_work[0]->pos.y < 0.0F) {
        stream_destroy();
        player_fall(0);
    }
}

/* @hack redundant: register on task, which keeps it in R14 (MOV R4,R14 at
 * 0x0c907c88); without it, task goes to the stack and 10 of 11 words
 * differ */
static void bottom_destroy(register struct task *task)
{
    world_object_free(task->extra);
    task->extra = NULL;
}

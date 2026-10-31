/* the barriers and the invincibility a player wears */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "draw/draw.h"
#include "effect/effect.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/util.h"

/* the barriers' sound as they start */
#define SE_BARRIER 6

/* the count on the last of the 1261 steps invincibility lasts. the step
 * that finds its count past this ends it. at the main loop's 60 steps a
 * second, it lasts about 21 seconds */
#define INVINCIBLE_LAST 1260

/* the last of the eleven steps a magnetic spark takes a new bolt and flash
 * on */
#define BOLT_LAST 10

/* a player effect's vars. the effect follows player, but reads the effects
 * worn and the height from player 0's record */
struct effect_vars {
    /* the player's slot, which player_give_barrier and the like set */
    Uint8 player;
    Uint8 unk_01[3];
    Sint32 timer; /* invincibility's steps so far */
};

/* a magnetic barrier's spark */
struct spark_vars {
    Uint8 bolt; /* which half of which lightning texture */
    Uint8 flash; /* which texture the shell flashes */
    Uint8 unk_02[6];
    Float fade; /* how bright it still is, from 1 */
};

#define EFFECT(work) ((struct effect_vars *)&(work)->vars)
#define SPARK(work) ((struct spark_vars *)&(work)->vars)

extern NJS_MODEL magnetic_spark_model;
extern NJS_MODEL barrier_spark_model;
extern NJS_MODEL invincibility_spark_model;
extern NJS_MODEL invincibility_ball_model;

static void magnetic_barrier_step(struct task *task);
static void magnetic_spark_step(struct task *task);
static void magnetic_spark_draw(struct task *task);
static void magnetic_barrier_destroy(struct task *task);
static void barrier_step(struct task *task);
static void barrier_spark_step(struct task *task);
static void barrier_spark_draw(struct task *task);
static void barrier_destroy(struct task *task);
static void invincibility_draw(struct task *task);
static void invincibility_step(struct task *task);
static void invincibility_spark_step(struct task *task);
static void invincibility_spark_draw(struct task *task);

/* the magnetic barrier's lightning, barria_t01 to t08, and its flashes, t09
 * to t11. each list ends in a second copy of its last */
static NJS_TEXLIST bolt_textures[9] = {
#include "effect/effect/bolt_textures.inc"
};
static NJS_TEXLIST flash_textures[4] = {
#include "effect/effect/flash_textures.inc"
};

/* the barrier's sparks, barria_n08 down to n01 and back */
static NJS_TEXLIST barrier_spark_textures[15] = {
#include "effect/effect/barrier_spark_textures.inc"
};

/* invincibility's sparks, muteki01 to 08, and its ball */
static NJS_TEXLIST invincibility_spark_textures[8] = {
#include "effect/effect/invincibility_spark_textures.inc"
};
static NJS_TEXLIST invincibility_ball_texture = {
#include "effect/effect/invincibility_ball_texture.inc"
};

/* a lightning bolt: a card of two triangles, from inside the shell up to it */
static NJS_POINT3 bolt_points[4] = {
#include "effect/effect/bolt_points.inc"
};
/* its colors, which the spark fades */
static NJS_COLOR bolt_colors[4] = {
#include "effect/effect/bolt_colors.inc"
};

/* place the effect on player, at the player's x scale. it sits up the
 * player's tilted axis, at 0.6 of player 0's height times that scale. scl.y
 * keeps that offset */
static void follow_player(struct task_work *work, struct task_work *player)
{
    NJS_POINT3 origin;

    origin.x = origin.y = origin.z = 0.0F;
    work->scl.x = player->scl.x;
    work->scl.y = player_get(0)->consts.height * work->scl.x * 0.6F;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &player->pos);
    TURN(njRotateZ, player->ang.z);
    TURN(njRotateX, player->ang.x);
    njTranslate(NULL, 0.0F, work->scl.y, 0.0F);
    njCalcPoint(NULL, &origin, &work->pos);
    njPopMatrix(1);
}

void magnetic_barrier_start(struct task *task)
{
    sound_play(SE_BARRIER, 0, 0, 0);
    task->destructor = magnetic_barrier_destroy;
    task->step = magnetic_barrier_step;
    magnetic_barrier_step(task);
}

static void magnetic_barrier_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[EFFECT(work)->player];
    struct task *child;
    struct task_work *spark;

    if (player == NULL || !(player_get(0)->effects & PLAYER_MAGNETIC_BARRIER)) {
        task_kill(task);
        return;
    }
    follow_player(work, player);
    if (rand() * RAND_SCALE > 0.7F) {
        if ((child = task_create_child(TASK_WORK, magnetic_spark_step, task)) !=
            NULL) {
            spark = child->work;
            spark->ang.x = RANDOM_ANGLE();
            spark->ang.y = RANDOM_ANGLE();
            SPARK(spark)->fade = 1.0F;
            child->draw = magnetic_spark_draw;
        }
    }
    task_step_children(task);
}

/* a spark takes a new bolt and flash each step up to BOLT_LAST. then it
 * fades from half bright by 0.025 a step, and goes once dark */
static void magnetic_spark_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;

    if (work->tick++ > BOLT_LAST) {
        /* @hack redundant: the else after the return, which loads the fade into
         * FR2 (0x8c046aa4, 0x8c046aa6); as two ifs, FR3, and 2 of 82 words
         * differ */
        if (SPARK(work)->fade <= 0.0F) {
            task_kill(task);
            return;
        } else if (SPARK(work)->fade > 0.5F) {
            SPARK(work)->fade = 0.5F;
        }
        SPARK(work)->fade -= 0.025F;
    } else {
        SPARK(work)->bolt = rand() * (16.0F * RAND_SCALE);
        SPARK(work)->flash = rand() * (3.0F * RAND_SCALE);
    }
    work->pos = parent->pos;
    work->scl.x = parent->scl.y * 0.2F;
    magnetic_spark_draw(task);
}

/* a patch of the shell flashes, darker as it fades, and a bolt reaches up
 * to it */
static void magnetic_spark_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3COL card;
    NJS_COLOR uv[4];
    Float shade;
    int texture;
    int left;
    Uint8 bright;

    if (frame_step != 0) {
        return;
    }
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njPushMatrix(NULL);
    shade = SPARK(work)->fade - 1.0F;
    draw_set_material(0.0F, shade, shade, shade);
    texture = SPARK(work)->flash;
    njSetTexture(&flash_textures[texture]);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateX, work->ang.x);
    njScale(NULL, work->scl.x, work->scl.x, work->scl.x);
    njSimpleDrawModel(&magnetic_spark_model);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    card.p = bolt_points;
    card.col = bolt_colors;
    card.tex = uv;
    card.num = 4;
    if (SPARK(work)->bolt & 1) {
        left = 128;
    } else {
        left = 0;
    }
    uv[0].tex.u = left;
    uv[0].tex.v = 0;
    uv[1].tex.u = left + 127;
    uv[1].tex.v = 0;
    uv[2].tex.u = left;
    uv[2].tex.v = 255;
    uv[3].tex.u = left + 127;
    uv[3].tex.v = 255;
    bright = SPARK(work)->fade * 255.0F;
    bolt_colors[0].argb.b = bolt_colors[0].argb.g = bolt_colors[0].argb.r =
        bright;
    bolt_colors[1].argb.b = bolt_colors[1].argb.g = bolt_colors[1].argb.r =
        bright;
    bolt_colors[2].argb.b = bolt_colors[2].argb.g = bolt_colors[2].argb.r =
        bright;
    bolt_colors[3].argb.b = bolt_colors[3].argb.g = bolt_colors[3].argb.r =
        bright;
    texture = SPARK(work)->bolt / 2;
    njSetTexture(&bolt_textures[texture]);
    njSetTextureNum(0);
    njDrawTriangle3D(&card, 2,
                     NJD_USE_TEXTURE | NJD_TRANSPARENT | NJD_FILL |
                         NJD_DRAW_CONNECTED);
    njPopMatrix(1);
}

static void magnetic_barrier_destroy(struct task *task)
{
}

void barrier_start(struct task *task)
{
    sound_play(SE_BARRIER, 0, 0, 0);
    task->destructor = barrier_destroy;
    task->step = barrier_step;
    barrier_step(task);
}

static void barrier_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[EFFECT(work)->player];
    struct task *child;
    struct task_work *spark;

    if (player == NULL || !(player_get(0)->effects & PLAYER_BARRIER)) {
        task_kill(task);
        return;
    }
    follow_player(work, player);
    if (rand() * RAND_SCALE > 0.7F) {
        if ((child = task_create_child(TASK_WORK, barrier_spark_step, task)) !=
            NULL) {
            spark = child->work;
            spark->ang.x = RANDOM_ANGLE();
            spark->ang.y = RANDOM_ANGLE();
            child->draw = barrier_spark_draw;
        }
    }
    task_step_children(task);
}

static void barrier_spark_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;

    if (work->tick >= 30) {
        task_kill(task);
        return;
    }
    work->pos = parent->pos;
    work->scl.x = parent->scl.y * 0.2F;
    barrier_spark_draw(task);
    work->tick++;
}

static void barrier_spark_draw(struct task *task)
{
    struct task_work *work = task->work;
    int frame;

    if (frame_step != 0) {
        return;
    }
    njPushMatrix(NULL);
    frame = work->tick >> 1;
    njSetTexture(&barrier_spark_textures[frame]);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateX, work->ang.x);
    njScale(NULL, work->scl.x, work->scl.x, work->scl.x);
    njSimpleDrawModel(&barrier_spark_model);
    njPopMatrix(1);
}

static void barrier_destroy(struct task *task)
{
}

void invincibility_start(struct task *task)
{
    task->destructor = barrier_destroy;
    /* @hack redundant: a store of the draw that the next store overwrites,
     * which SHC deletes but whose load it keeps (MOV.L at 0x8c046faa);
     * without it, the function runs 2 bytes short and 3 of 6 words differ */
    task->step = invincibility_draw;
    task->step = invincibility_step;
    invincibility_step(task);
}

static void invincibility_draw(struct task *task)
{
    struct task_work *work = task->work;

#ifdef GCC
    /* the task has no callback for the second picture's pass, so step 1
     * draws the ball for that picture where port.h's FRAME_DRAWS says.
     * where it says not, the ball shows in the first picture alone */
    if (!FRAME_DRAWS()) {
        return;
    }
    present_untimed(TRUE);
#else
    if (frame_step != 0) {
        return;
    }
#endif
    njPushMatrix(NULL);
    njSetTexture(&invincibility_ball_texture);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateX, work->ang.x);
    njScale(NULL, work->scl.x, work->scl.x, work->scl.x);
    njSimpleDrawModel(&invincibility_ball_model);
    njPopMatrix(1);
#ifdef GCC
    present_untimed(FALSE);
#endif
}

static void invincibility_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[EFFECT(work)->player];
    struct player *record = player_get(0);
    struct task *child;
    struct task_work *spark;
    int i;

    if (player == NULL || EFFECT(work)->timer++ > INVINCIBLE_LAST) {
        record->effects &= ~PLAYER_INVINCIBLE;
        task_kill(task);
        return;
    }
    record->effects |= PLAYER_INVINCIBLE;
    follow_player(work, player);
    for (i = 0; i < 3; i++) {
        if (rand() * RAND_SCALE > 0.7F) {
            if ((child = task_create_child(TASK_WORK, invincibility_spark_step,
                                           task)) != NULL) {
                spark = child->work;
                spark->ang.x = RANDOM_ANGLE();
                spark->ang.y = RANDOM_ANGLE();
                child->draw = invincibility_spark_draw;
            }
        }
    }
    invincibility_draw(task);
    task_step_children(task);
}

static void invincibility_spark_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;

    if (work->tick >= 32) {
        task_kill(task);
        return;
    }
    work->pos = parent->pos;
    work->scl.x = parent->scl.y * 0.2F;
    invincibility_spark_draw(task);
    work->tick++;
}

static void invincibility_spark_draw(struct task *task)
{
    struct task_work *work = task->work;
    int frame;

    if (frame_step != 0) {
        return;
    }
    njPushMatrix(NULL);
    frame = (work->tick >> 1) & 7;
    njSetTexture(&invincibility_spark_textures[frame]);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateX, work->ang.x);
    njScale(NULL, work->scl.x, work->scl.x, work->scl.x);
    njSimpleDrawModel(&invincibility_spark_model);
    njPopMatrix(1);
}

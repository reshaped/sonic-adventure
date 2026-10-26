/* Chaos 4's helpers, and the task that plays an effect sprite's frames
 * once */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos4/boss.h"
#include "b_chaos4/util.h"
#include "camera/camera.h"
#include "chaos/emeralds.h"
#include "chaos/models.h"
#include "draw/motion.h"
#include "event/script.h"
#include "hud/hud.h"
#include "hud/life_gauge.h"
#include "object/tikal.h"
#include "player/player.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 50
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 9
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 9
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 9
#include "b_chaos4/util/text.inc"

/* the share of what is left of his turn that chaos4_turn makes a call */
#define TURN_SHARE 0.3F

/* the emeralds chaos4_emeralds_create makes */
#define EMERALDS 4

/* the splash plays SPLASH_FRAMES frames. each step's advance falls short
 * of a whole frame by a random share of SPLASH_JITTER */
#define SPLASH_FRAMES 8
#define SPLASH_JITTER 0.5F

/* each effect sprite draws SPRITE_SIZE times its size, colored a gray of
 * SPRITE_GRAY at SPRITE_ALPHA, and added to what is behind it */
#define SPRITE_SIZE 5.0F
#define SPRITE_GRAY 0.25F
#define SPRITE_ALPHA 0.75F

/* chaos4_hint counts the time in whole seconds times 100, plus frames, so
 * HINT_TIME is a minute. HINT_HEALTH is the 5 health he starts with, of
 * which BOSS_WAIT takes 1 against Tails before chaos4_hint first runs. the
 * hint's one page shows for HINT_STEPS steps, and Tikal speaks voice
 * HINT_VOICE */
#define HINT_TIME 6000
#define HINT_HEALTH 5.0F
#define HINT_STEPS 150
#define HINT_VOICE 0xb9

/* an effect sprite's task's work, which overlays struct task_work with
 * fields of its own. speed lies where task_work's ang does */
struct effect_sprite_work {
    Sint8 mode;
    Sint8 frames; /* the sprite's frame count */
    Uint8 unk_02[6];
    Float frame; /* the frame playing, which draws truncated */
    NJS_SPRITE *sprite;
    /* the most a step's advance falls short of a whole frame, from 0 to 1 */
    Float jitter;
    NJS_VECTOR speed;
    NJS_POINT3 pos;
};

enum effect_sprite_mode { SPRITE_START, SPRITE_PLAY };

static void effect_sprite_create(NJS_SPRITE *sprite, Sint8 frames,
                                 NJS_POINT3 *pos, NJS_VECTOR *speed,
                                 Float jitter);
static void effect_sprite_step(struct task *task);
static void effect_sprite_draw(struct task *task);

/* each emerald's slot, and the boss point it grows at */
static const int emerald_slots[EMERALDS] = {0, 1, 2, 3};
static const int emerald_points[EMERALDS] = {0, 1, 2, 3};

static NJS_ARGB plain_material = {0.0F, 0.0F, 0.0F, 0.0F};

/* the CHAOS4_SHIBUKI pvm's textures, the splash's frames */
static NJS_TEXNAME splash_textures[SPLASH_FRAMES] = {
    {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0}, {TEXT_5, 0, 0},
    {TEXT_6, 0, 0}, {TEXT_7, 0, 0}, {TEXT_8, 0, 0}, {TEXT_9, 0, 0},
};
NJS_TEXLIST chaos4_shibuki_texlist = {
#include "b_chaos4/util/chaos4_shibuki_texlist.inc"
};

static NJS_TEXANIM splash_texanims[SPLASH_FRAMES] = {
#include "b_chaos4/util/splash_texanims.inc"
};
static NJS_TEXANIM drop_texanim[1] = {
#include "b_chaos4/util/drop_texanim.inc"
};

static const struct script_page hint_pages[2] = {
    {TEXT_1, HINT_STEPS},
    {NULL, 0},
};

/* TRUE once the hint has shown, until the module loads again */
static Bool hint_shown = FALSE;

NJS_SPRITE chaos4_splash_sprite;
NJS_SPRITE chaos4_drop_sprite;

void chaos4_turn(struct task_work *work)
{
    struct chaos4 *record = CHAOS4_VARS(work)->record;
    struct task_work *player;
    Angle ang;

    switch (record->turn) {
    case TURN_NONE:
        return;
    case TURN_HEADING:
        ang = record->heading;
        break;
    case TURN_PLAYER:
        player = player_work[0];
        ang = (Angle)(atan2f(player->pos.z - work->pos.z,
                             player->pos.x - work->pos.x) *
                      ANGLE_PER_RADIAN);
        break;
    default:
        return;
    }
    work->ang.y += angle_difference(work->ang.y, ang) * TURN_SHARE;
}

void chaos4_material_reset(void)
{
    njSetConstantMaterial(&plain_material);
}

void chaos4_emeralds_create(void)
{
    int i;

    for (i = 0; i < EMERALDS; i++) {
        chaos_emerald_create(&boss_points[emerald_points[i]].pos,
                             emerald_slots[i]);
    }
}

void chaos4_sprites_init(void)
{
    sprite_init(&chaos4_splash_sprite, &chaos4_shibuki_texlist,
                splash_texanims);
    sprite_init(&chaos4_drop_sprite, &chaos_effect_texlist, drop_texanim);
}

void chaos4_splash_create(NJS_POINT3 *pos)
{
    sprite_init(&chaos4_splash_sprite, &chaos4_shibuki_texlist,
                splash_texanims);
    effect_sprite_create(&chaos4_splash_sprite, SPLASH_FRAMES, pos, NULL,
                         SPLASH_JITTER);
}

/* start a task that plays the sprite's frames once at pos, moving by speed
 * each step, or still when speed is NULL. it copies pos and speed */
static void effect_sprite_create(NJS_SPRITE *sprite, Sint8 frames,
                                 NJS_POINT3 *pos, NJS_VECTOR *speed,
                                 Float jitter)
{
    struct task *task;
    struct effect_sprite_work *work;

    if ((task = task_create(TASK_WORK, 5, effect_sprite_step)) != NULL) {
        work = (struct effect_sprite_work *)task->work;
        work->sprite = sprite;
        if (speed != NULL) {
            VECTOR_COPY(&work->speed, speed);
        } else {
            /* @hack type: the speed cleared through task_work's ang, which
             * overlays it, so each store is an integer 0 (0x0c904d40 to
             * 0x0c904d46); as Float stores, effect_sprite_create runs 2 bytes
             * long and 48 of 68 words differ */
            ((struct task_work *)work)->ang.x =
                ((struct task_work *)work)->ang.y =
                    ((struct task_work *)work)->ang.z = 0;
        }
        work->frames = frames;
        work->jitter = jitter;
        work->frame = 0.0F;
        VECTOR_COPY(&work->pos, pos);
        task->step = effect_sprite_step;
        task->draw = effect_sprite_draw;
    }
}

/* the first step only starts the play. each step after it advances frame
 * by 1 less a random share of jitter, and moves pos by speed. the step
 * that reaches the last frame holds it there and kills the task */
static void effect_sprite_step(struct task *task)
{
    struct effect_sprite_work *work = (struct effect_sprite_work *)task->work;

    switch (work->mode) {
    case SPRITE_START:
        work->mode = SPRITE_PLAY;
        break;
    case SPRITE_PLAY:
        work->frame += 1.0F - (work->jitter * rand() * RAND_SCALE);
        if ((int)work->frame >= work->frames - 1) {
            work->frame = work->frames - 1;
            task_kill(task);
        }
        VECTOR_SUM(&work->pos, &work->pos, &work->speed);
        break;
    }
    effect_sprite_draw(task);
}

/* on a frame's first step, draw the sprite's frame at pos, facing the
 * camera. the draw then sets the constant material back to 0, and both
 * blend modes to NJD_COLOR_BLENDING_BOTHSRCALPHA */
static void effect_sprite_draw(struct task *task)
{
    struct effect_sprite_work *work = (struct effect_sprite_work *)task->work;
    Angle ang;
    NJS_ARGB color;

    if (frame_step == 0) {
        njPushMatrix(NULL);
        color.a = SPRITE_ALPHA;
        color.r = SPRITE_GRAY;
        color.g = SPRITE_GRAY;
        color.b = SPRITE_GRAY;
        njSetConstantMaterial(&color);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, camera_work->ang.y, ang);
        TURN_VIA(njRotateX, camera_work->ang.x, ang);
        njScale(NULL, SPRITE_SIZE, SPRITE_SIZE, SPRITE_SIZE);
        njDrawSprite3D(work->sprite, (int)work->frame,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        color.a = 0.0F;
        color.r = 0.0F;
        color.g = 0.0F;
        color.b = 0.0F;
        njSetConstantMaterial(&color);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_BOTHSRCALPHA);
        njPopMatrix(1);
    }
}

void chaos4_hint(void)
{
    Bool show = FALSE;
    Uint8 minutes;
    Uint8 seconds;
    Uint8 frames;
    int time;

    if (hint_shown) {
        return;
    }
    if (stage_get_retries() > 0) {
        show = TRUE;
    } else if (life_gauge_health >= HINT_HEALTH) {
        time_get(&minutes, &seconds, &frames);
        time = (minutes * 6000) + (seconds * 100) + frames;
        if ((time < 0 ? -time : time) > HINT_TIME) {
            show = TRUE;
        }
    }
    if (show) {
        hint_shown = TRUE;
        tikal_hint_create((struct script_page *)hint_pages, HINT_VOICE);
    }
}

void chaos4_hint_end(void)
{
    tikal_hint_end();
}

/* the sparks of an animal a player frees, and the flash as its barrier goes */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "animal/sparks.h"
#include "effect/hit.h" /* NOLINT(misc-include-cleaner) */
#include "effect/lens.h" /* NOLINT(misc-include-cleaner) */
#include "object/tikal_light.h" /* NOLINT(misc-include-cleaner) */
#include "system/task.h"
#include "system/util.h"

/* a burst's sparks. the first SPARK_SLOW_COUNT are slow, and the rest fast */
#define SPARK_COUNT 14
#define SPARK_SLOW_COUNT 7

/* the slow and the fast sparks' speeds, and the steps a fast spark waits
 * before it moves and shows */
#define SLOW_SPEED 1.5F
#define FAST_SPEED 3.0F
#define FAST_DELAY 10

/* what a spark's fade gains each step after its delay. the fade reaches 1,
 * which ends the spark, on the 34th step after the delay */
#define SPARK_FADE_STEP (1.8F / 60.0F)

/* the flash sprite's last frame */
#define FLASH_LAST 3

enum spark_sprite { SPARK_SLOW, SPARK_FAST };

/* the flash's work->mode. FLASH_FIRST shows frame 0 until the flash's fade
 * passes 0.5. FLASH_PLAY shows the next frame each step up to FLASH_LAST,
 * holds that frame, and ends the flash when the fade reaches 1 */
enum flash_mode { FLASH_FIRST, FLASH_PLAY };

/* a spark's work->vars. its spark_color is work->mode, and its velocity
 * work->scl */
struct spark_vars {
    Float fade; /* from 0, and gone at 1 */
    Sint16 sprite; /* a spark_sprite */
    Sint16 delay; /* steps before it moves and shows */
};

/* the flash's work->vars. its spark_color is work->submode */
struct flash_vars {
    Float fade; /* from 0, and gone at 1 */
    Sint16 frame;
    Sint16 timer; /* steps it has run, which nothing reads */
};

#define SPARK(work) ((struct spark_vars *)&(work)->vars)
#define FLASH(work) ((struct flash_vars *)&(work)->vars)

static void animal_spark_step(struct task *task);
static void animal_flash_step(struct task *task);
static NJS_ARGB animal_spark_color(int color, Float alpha, Float level);

/* these are const, so SHC puts them in section C, with the constants.
 * Ninja's prototypes take no const, so the calls cast it away */

/* the slow spark's texture is tical_l, and the fast one's hit_l01 */
static const NJS_TEXANIM animal_slow_texanims[1] = {
#include "animal/sparks/animal_slow_texanims.inc"
};
static const NJS_SPRITE animal_slow_sprite = {
#include "animal/sparks/animal_slow_sprite.inc"
};
static const NJS_TEXANIM animal_fast_texanims[1] = {
#include "animal/sparks/animal_fast_texanims.inc"
};
static const NJS_SPRITE animal_fast_sprite = {
#include "animal/sparks/animal_fast_sprite.inc"
};
/* the flash's frames, lens05 down to lens02 */
static const NJS_TEXANIM animal_flash_texanims[4] = {
#include "animal/sparks/animal_flash_texanims.inc"
};
static const NJS_SPRITE animal_flash_sprite = {
#include "animal/sparks/animal_flash_sprite.inc"
};

/* the slow sparks fly off in any direction, each after a random 0 to 4
 * steps. the fast ones fly off after FAST_DELAY steps, each at a random
 * heading. they share one random pitch, up to 30 degrees above or below
 * level */
void animal_sparks_create(NJS_POINT3 *pos, int color, int grounded)
{
    struct task *task;
    struct task_work *work;
    Angle fast_pitch;
    Angle pitch;
    Angle yaw;
    Float across;
    int i;

    fast_pitch = RANDOM(-NJM_DEG_ANG(30), NJM_DEG_ANG(30));
    for (i = 0; i < SPARK_COUNT; i++) {
        /* @hack spelling: task assigned in the test, which keeps pos on the
         * stack (MOV.L R4,@R15 at 0x8c01899c); as a statement of its own,
         * pos takes R8, the function runs 6 bytes short and 123 of 175
         * words differ */
        if ((task = task_create(TASK_WORK, 6, animal_spark_step)) != NULL) {
            work = task->work;
            pitch = RANDOM_ANGLE();
            yaw = RANDOM_ANGLE();
            SPARK(work)->fade = 0.0F;
            /* @hack redundant: the copy in a loop of one pass, which keeps
             * the loop round it rolled, with one call of task_create
             * (0x8c0189ce); as the copy alone, SHC unrolls that loop and
             * sparks.c's code runs 1024 bytes long, count unmeasured */
            do {
                VECTOR_COPY(&work->pos, pos);
            } while (0);
            work->mode = color;
            if (i < SPARK_SLOW_COUNT) {
                SPARK(work)->delay = RANDOM(0.0F, 5.0F);
                SPARK(work)->sprite = SPARK_SLOW;
                work->scl.y = njSin(pitch) * SLOW_SPEED;
                across = njCos(pitch) * SLOW_SPEED;
                work->scl.x = across * njCos(yaw);
                work->scl.z = -(across * njSin(yaw));
            } else {
                SPARK(work)->delay = FAST_DELAY;
                SPARK(work)->sprite = SPARK_FAST;
                work->scl.y = njSin(fast_pitch) * FAST_SPEED;
                across = njCos(fast_pitch) * FAST_SPEED;
                work->scl.x = across * njCos(yaw);
                work->scl.z = -(across * njSin(yaw));
            }
            if (grounded) {
                if (work->scl.y < 0.0F) {
                    work->scl.y = -work->scl.y;
                }
            }
        }
    }
}

void animal_flash_create(NJS_POINT3 *pos, int color)
{
    struct task *task;
    struct task_work *work;

    /* @hack spelling: task assigned in the test, which tests it in R0 and
     * copies it to R4 in BT/S's slot (TST R0,R0 at 0x8c018af2); as a
     * statement of its own, SHC copies it first and tests R4, and 3 of 40
     * words differ */
    if ((task = task_create(TASK_WORK, 6, animal_flash_step)) != NULL) {
        work = task->work;
        VECTOR_COPY(&work->pos, pos);
        FLASH(work)->frame = 0;
        work->mode = FLASH_FIRST;
        work->submode = color;
        FLASH(work)->fade = 0.0F;
        FLASH(work)->timer = 0;
    }
}

/* after its delay, the spark falls, slows and fades. the channels its
 * spark_color doesn't fill show at a level that falls from 1 to 0 over the
 * first two thirds of the fade */
static void animal_spark_step(struct task *task)
{
    struct task_work *work;
    NJS_VECTOR drag;
    NJS_ARGB color;
    Float level;
    NJS_SPRITE *sprite;

    work = task->work;
    if (SPARK(work)->delay-- > 0) {
        return;
    }
    SPARK(work)->fade += SPARK_FADE_STEP;
    if (SPARK(work)->fade >= 1.0F) {
        task_kill(task);
        return;
    }
    work->scl.y -= 0.02F;
    VECTOR_SCALE(&drag, &work->scl, 0.2F);
    VECTOR_SUB(&work->scl, &drag);
    VECTOR_ADD(&work->pos, &work->scl);
    level = 1.0F - (SPARK(work)->fade * 1.5F);
    if (level < 0.0F) {
        level = 0.0F;
    }
    color = animal_spark_color(work->mode, 1.0F - SPARK(work)->fade, level);
    switch (SPARK(work)->sprite) {
    case SPARK_SLOW:
        sprite = (NJS_SPRITE *)&animal_slow_sprite;
        break;
    case SPARK_FAST:
    default:
        sprite = (NJS_SPRITE *)&animal_fast_sprite;
        break;
    }
    njSetConstantMaterial(&color);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njDrawSprite3D(sprite, 0,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR);
    njPopMatrix(1);
    color.a = color.r = color.g = color.b = 1.0F;
    njSetConstantMaterial(&color);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
}

static void animal_flash_step(struct task *task)
{
    struct task_work *work;
    NJS_ARGB color;
    Float level;

    work = task->work;
    FLASH(work)->timer++;
    switch (work->mode) {
    case FLASH_FIRST:
        FLASH(work)->fade += 0.02F;
        if (FLASH(work)->fade > 0.5F) {
            work->mode = FLASH_PLAY;
        }
        break;
    case FLASH_PLAY:
        FLASH(work)->frame++;
        if (FLASH(work)->frame > FLASH_LAST) {
            FLASH(work)->frame = FLASH_LAST;
        }
        FLASH(work)->fade += 0.015F;
        if (FLASH(work)->fade >= 1.0F) {
            task_kill(task);
            return;
        }
        break;
    }
    level = 1.0F - (FLASH(work)->fade * 1.8F);
    if (level < -0.8F) {
        level = -0.8F;
    }
    color = animal_spark_color(work->submode, 1.0F - FLASH(work)->fade, level);
    njSetConstantMaterial(&color);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njDrawSprite3D((NJS_SPRITE *)&animal_flash_sprite, FLASH(work)->frame,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR);
    njPopMatrix(1);
    color.a = color.r = color.g = color.b = 1.0F;
    njSetConstantMaterial(&color);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
}

/* the ARGB of color, a spark_color, at alpha, with level in the channels
 * the color doesn't fill. any other color gives SPARK_MINT's, which takes no
 * level */
static NJS_ARGB animal_spark_color(int color, Float alpha, Float level)
{
    NJS_ARGB argb;

    argb.a = alpha;
    switch (color) {
    case SPARK_RED:
        argb.r = 1.0F;
        argb.b = level;
        argb.g = level;
        break;
    case SPARK_GREEN:
        argb.r = level;
        argb.b = level;
        argb.g = 1.0F;
        break;
    case SPARK_BLUE:
        argb.r = level;
        argb.b = 1.0F;
        argb.g = level;
        break;
    case SPARK_WHITE:
        argb.r = 1.0F;
        argb.b = 1.0F;
        argb.g = 1.0F;
        break;
    case SPARK_YELLOW:
        argb.r = 1.0F;
        argb.b = level;
        argb.g = 1.0F;
        break;
    case SPARK_MAGENTA:
        argb.r = 1.0F;
        argb.b = 1.0F;
        argb.g = level;
        break;
    case SPARK_CYAN:
        argb.r = level;
        argb.b = 1.0F;
        argb.g = 1.0F;
        break;
    case SPARK_MINT:
    default:
        argb.r = 0.5F;
        argb.b = 0.7F;
        argb.g = 1.0F;
        break;
    }
    return argb;
}

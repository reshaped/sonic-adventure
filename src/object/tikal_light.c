/* Tikal's light: a ball of light that bobs at a point and sheds sparks,
 * and the red flashes it leaves behind */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "event/play.h"
#include "object/common.h"
#include "object/tikal_light.h"
#include "system/frame.h"
#include "system/math.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 7
#define TEXT_7_LENGTH 7
#define TEXT_8_LENGTH 7
#define TEXT_9_LENGTH 7
#include "object/tikal_light/text.inc"

/* a light's view of task_work, which a flash takes too */
struct light_work {
    Sint8 mode;
    Sint8 submode;
    Uint8 id;
    /* the count of steps LIGHT_TEXANIM turns into the light's texanim. it
     * goes back to 0 once the texanim would pass the last */
    Sint8 tick;
    Sint16 flags;
    Sint16 timer; /* a flash's steps left */
    NJS_ARGB color;
    /* the phases of its bob: along x and z, which move as one, and along y */
    Angle bob_across;
    Angle bob_up;
    NJS_POINT3 pos;
    Float scale;
    NJS_POINT3 *origin; /* the point it bobs at */
    /* the owner's pointer to the light, which the light clears as it's
     * destroyed, or NULL for none */
    struct task **holder;
    struct collision_work *collision;
    struct event_work *event;
};

struct spark_work {
    Sint8 mode;
    Sint8 submode;
    Uint8 id;
    Uint8 tick;
    Sint16 flags;
    Sint16 timer; /* steps left */
    NJS_ARGB color;
    Angle pulse; /* the phase that sets its blue */
    Uint8 unk_1c[4];
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    struct collision_work *collision;
    struct event_work *event;
};

#define LIGHT(task) ((struct light_work *)(task)->work)
#define SPARK(task) ((struct spark_work *)(task)->work)

/* the steps in a second at tikal_light_pace, and in 0.33 of a second */
#define STEPS_PER_SECOND (60 / tikal_light_pace)
#define FLASH_STEPS (STEPS_PER_SECOND * 0.33F)

/* the light's texanims, and the one it shows at tick. it moves on once a
 * frame: every other step while tikal_light_pace is 1, and every step
 * while it's 2 */
#define LIGHT_TEXANIMS 8
#define LIGHT_TEXANIM(tick) ((tikal_light_pace == 1) ? (tick) >> 1 : (tick))

/* the sprite scales that make a light's or a flash's scale its radius.
 * the light's texanims are 64 across, and the spark's 16, which a flash
 * draws too. a spark's radius is SPARK_RADIUS */
#define LIGHT_SPRITE_SCALE (1.0F / 32.0F)
#define SPARK_SPRITE_SCALE (1.0F / 8.0F)
#define SPARK_RADIUS 0.25F
#define FULL_SCALE 4.0F /* a new light's scale, and every flash's */

/* the light bobs BOB_REACH times tikal_light_pace each way along each
 * axis. it sheds sparks while its alpha is over SHED_ALPHA and its scale
 * over SHED_SCALE */
#define BOB_REACH 0.25F
#define SHED_ALPHA 0.05F
#define SHED_SCALE 0.25F

/* the fall a step, times tikal_light_pace, at which a spark stops falling
 * faster */
#define FALL_LIMIT 0.4F

#define FLASH_ALPHA 0.3F /* a flash's alpha as it starts */

static void tikal_spark_init(struct task *task, NJS_POINT3 *pos, Float scale,
                             Float alpha);
static void tikal_spark_step(struct task *task);
static void tikal_spark_draw(struct task *task);
static void tikal_spark_destroy(struct task *task);
static void tikal_light_init(struct task *task);
static void tikal_light_step(struct task *task);
static void tikal_light_draw(struct task *task);
static void tikal_light_destroy(struct task *task);
static void tikal_light_draw_sprite(NJS_SPRITE *sprite, NJS_POINT3 *pos,
                                    NJS_ARGB *color, int texanim, Float scale);
static void tikal_flash_init(struct task *task, NJS_POINT3 *pos,
                             NJS_ARGB *color, int tick);
static void tikal_flash_step(struct task *task);
static void tikal_flash_draw(struct task *task);

/* how many of a stage's steps each step of a light, spark or flash stands
 * for. a stage runs two steps a frame, and an event that has control runs
 * one. tikal_light_create sets 1, or 2 while an event has control */
static int tikal_light_pace = 1;

static NJS_TEXNAME hint_textures[9] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0}, {TEXT_6, 0, 0},
    {TEXT_7, 0, 0}, {TEXT_8, 0, 0}, {TEXT_9, 0, 0},
};

static NJS_TEXANIM tikal_light_texanims[LIGHT_TEXANIMS] = {
#include "object/tikal_light/tikal_light_texanims.inc"
};

static NJS_SPRITE tikal_light_sprite = {
#include "object/tikal_light/tikal_light_sprite.inc"
};

/* the spark's one texanim, which a flash draws too */
static NJS_TEXANIM tikal_spark_texanim = {
#include "object/tikal_light/tikal_spark_texanim.inc"
};

static NJS_SPRITE tikal_spark_sprite = {
#include "object/tikal_light/tikal_spark_sprite.inc"
};

/* a blank constant material, which each draw sets once it's done */
static NJS_ARGB tikal_light_no_material = {0.0F, 0.0F, 0.0F, 0.0F};

NJS_TEXLIST hint_texlist = {
#include "object/tikal_light/hint_texlist.inc"
};

struct task *tikal_light_create(struct task *parent, struct task **holder,
                                NJS_POINT3 *origin)
{
    struct task *task;
    struct light_work *work;

    /* @hack spelling: task assigned in the test, which tests
     * task_create_child's R0 with the copy to R14 in the BT/S's slot
     * (0x8c062ff8, 0x8c062ffc); assigned apart, SHC tests R14 and 3 of 37
     * words differ */
    if ((task = task_create_child(TASK_WORK, tikal_light_step, parent)) !=
        NULL) {
        work = LIGHT(task);
        tikal_light_init(task);
        work->holder = holder;
        work->origin = origin;
        tikal_light_pace = event_in_control() ? 2 : 1;
    }
    return task;
}

void tikal_light_set(struct task *light, Float scale, Float alpha)
{
    struct light_work *work;

    if (light == NULL) {
        return;
    }
    work = LIGHT(light);
    work->color.a = alpha;
    work->scale = scale;
}

void tikal_light_flash(struct task *light)
{
    struct light_work *work;
    struct task *task;

    if (light != NULL) {
        work = LIGHT(light);
        /* @hack spelling: task assigned in the test, which tests task_create's
         * R0 with the copy to R4 in the BT/S's slot (0x8c063050,
         * 0x8c063054); assigned apart, SHC tests R4 and 3 of 25 words
         * differ */
        if ((task = task_create(TASK_WORK, 6, tikal_flash_step)) != NULL) {
            tikal_flash_init(task, &work->pos, &work->color, work->tick);
        }
    }
}

void tikal_light_move_toward(NJS_POINT3 *pos, NJS_POINT3 *to, Float distance)
{
    njSubVector(to, pos);
    vector_unit(to, to);
    to->x *= distance;
    to->y *= distance;
    to->z *= distance;
    njAddVector(pos, to);
}

/* a white spark at most a second long, a little below the light, drifting
 * across at random and falling */
static void tikal_spark_init(struct task *task, NJS_POINT3 *pos, Float scale,
                             Float alpha)
{
    struct spark_work *work = SPARK(task);

    /* go no further unless spark_work keeps these fields where task_work
     * does, as tikal_light_init asks of light_work */
    /* @hack redundant: pos tested twice, as the image tests it (ADD #32,R2 at
     * 0x8c063114 and 0x8c063148); tested once, 102 of 135 words differ */
    /* NOLINTBEGIN(misc-redundant-expression) */
    if (!WORK_SAME_PLACE(struct spark_work, pos) ||
        !WORK_SAME_PLACE(struct spark_work, id) ||
        !WORK_SAME_PLACE(struct spark_work, collision) ||
        !WORK_SAME_PLACE(struct spark_work, event) ||
        !WORK_SAME_PLACE(struct spark_work, pos) ||
        !WORK_SAME_PLACE(struct spark_work, flags)) {
        return;
    }
    /* NOLINTEND(misc-redundant-expression) */
    work->pos = *pos;
    work->pos.y -= scale * 0.4F;
    work->timer = STEPS_PER_SECOND;
    work->speed.x = (0.5F - (rand() * RAND_SCALE)) * 0.1F;
    work->speed.z = (0.5F - (rand() * RAND_SCALE)) * 0.1F;
    work->speed.y = -((Float)tikal_light_pace * rand() * (0.1F * RAND_SCALE));
    work->color.a = alpha;
    work->color.r = work->color.g = work->color.b = 1.0F;
    work->pulse = NJM_DEG_ANG(90);
    task->draw = tikal_spark_draw;
    task->destructor = tikal_spark_destroy;
}

/* drag slows it across, and it falls faster each step until FALL_LIMIT. it
 * fades from white through yellow to red, its blue dipping to 0 on the
 * way */
static void tikal_spark_step(struct task *task)
{
    struct spark_work *work = SPARK(task);

    work->speed.x *= (tikal_light_pace == 1) ? 0.999F : 0.999F * 0.999F;
    work->speed.z *= (tikal_light_pace == 1) ? 0.999F : 0.999F * 0.999F;
    if (work->speed.y > -(tikal_light_pace * FALL_LIMIT)) {
        work->speed.y += (Float)tikal_light_pace * tikal_light_pace * -0.01F;
    }
    njAddVector(&work->pos, &work->speed);
    work->pulse += NJM_DEG_ANG(270) / STEPS_PER_SECOND;
    /* NOLINTNEXTLINE(bugprone-integer-division) */
    work->color.a -= 1.0F / STEPS_PER_SECOND;
    work->color.g = work->color.a * work->color.a;
    work->color.r = work->color.a - work->color.g + 1.0F;
    work->color.b = (njSin(work->pulse) + 1.0F) * 0.5F;
    if (--work->timer == 0 || work->color.a < 0.0F) {
        task_destroy(task);
    } else {
        tikal_spark_draw(task);
    }
}

static void tikal_spark_draw(struct task *task)
{
    struct spark_work *work = SPARK(task);

    tikal_light_draw_sprite(&tikal_spark_sprite, &work->pos, &work->color, 0,
                            SPARK_RADIUS * SPARK_SPRITE_SCALE);
}

static void tikal_spark_destroy(struct task *task)
{
}

static void tikal_light_init(struct task *task)
{
    struct light_work *work = LIGHT(task);

    /* @hack redundant: pos tested twice, as the image tests it (ADD #32,R2 at
     * 0x8c0633e6 and 0x8c063418); tested once, 34 of 59 words differ */
    /* NOLINTBEGIN(misc-redundant-expression) */
    if (!WORK_SAME_PLACE(struct light_work, pos) ||
        !WORK_SAME_PLACE(struct light_work, id) ||
        !WORK_SAME_PLACE(struct light_work, collision) ||
        !WORK_SAME_PLACE(struct light_work, event) ||
        !WORK_SAME_PLACE(struct light_work, pos) ||
        !WORK_SAME_PLACE(struct light_work, flags)) {
        return;
    }
    /* NOLINTEND(misc-redundant-expression) */
    work->color.a = work->color.r = work->color.g = work->color.b = 1.0F;
    work->scale = FULL_SCALE;
    task->step = tikal_light_step;
    task->draw = tikal_light_draw;
    task->destructor = tikal_light_destroy;
}

/* bob at the origin and count the texanims on. while the light is bright
 * and big enough, it sheds two sparks a frame */
static void tikal_light_step(struct task *task)
{
    struct light_work *work = LIGHT(task);
    struct task *spark;
    Float sine;
    Float cosine;

    work->bob_across += tikal_light_pace << 10;
    work->bob_up += tikal_light_pace << 12;
    _builtin_fsca(work->bob_across, &sine, &cosine);
    work->pos.x = work->pos.z = tikal_light_pace * sine * BOB_REACH;
    _builtin_fsca(work->bob_up, &sine, &cosine);
    work->pos.y = tikal_light_pace * sine * BOB_REACH;
    /* @hack spelling: tick counted up in the test, which has SHC add and store
     * it in each arm of LIGHT_TEXANIM's conditional (0x8c0634d0 to 0x8c0634d4,
     * and 0x8c0634de to 0x8c0634e4); counted up apart, it does that once ahead
     * of the pace test, the function runs 4 bytes short and 73 of 133 words
     * differ */
    /* NOLINTNEXTLINE(bugprone-inc-dec-in-conditions) */
    if (LIGHT_TEXANIM(++work->tick) > LIGHT_TEXANIMS - 1) {
        work->tick = 0;
    }
    njAddVector(&work->pos, work->origin);
    if (work->color.a > SHED_ALPHA && work->scale > SHED_SCALE) {
        /* @hack spelling: spark assigned in the test, which tests
         * task_create's R0 with the copy to R4 in the BT/S's slot
         * (0x8c06351a, 0x8c06351e); assigned apart, SHC tests R4 and 3 of
         * 133 words differ */
        if ((spark = task_create(TASK_WORK, 6, tikal_spark_step)) != NULL) {
            tikal_spark_init(spark, &work->pos, work->scale, work->color.a);
        }
        if (tikal_light_pace == 2) {
            /* @hack spelling: spark assigned in the test, as above (0x8c06353e,
             * 0x8c063542); assigned apart, 3 of 133 words differ */
            if ((spark = task_create(TASK_WORK, 6, tikal_spark_step)) != NULL) {
                tikal_spark_init(spark, &work->pos, work->scale, work->color.a);
            }
        }
    }
    tikal_light_draw(task);
}

static void tikal_light_draw(struct task *task)
{
    struct light_work *work = LIGHT(task);

    tikal_light_draw_sprite(&tikal_light_sprite, &work->pos, &work->color,
                            LIGHT_TEXANIM(work->tick),
                            work->scale * LIGHT_SPRITE_SCALE);
}

static void tikal_light_destroy(struct task *task)
{
    struct light_work *work = LIGHT(task);

    if (work->holder) {
        *work->holder = NULL;
    }
}

/* draw sprite's texanim at pos, at scale and in color, added onto what lies
 * behind it. it draws only on a frame's first step */
static void tikal_light_draw_sprite(NJS_SPRITE *sprite, NJS_POINT3 *pos,
                                    NJS_ARGB *color, int texanim, Float scale)
{
    if (!frame_step) {
        njSetConstantMaterial(color);
        sprite->p = *pos;
        sprite->sx = scale;
        sprite->sy = scale;
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njDrawSprite3D(sprite, texanim,
                       NJD_SPRITE_COLOR | NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_BOTHSRCALPHA);
        njSetConstantMaterial(&tikal_light_no_material);
    }
}

/* a flash at pos in color's red alone, at FLASH_ALPHA, with FLASH_STEPS on
 * its timer. it keeps the light's tick, which nothing reads */
static void tikal_flash_init(struct task *task, NJS_POINT3 *pos,
                             NJS_ARGB *color, int tick)
{
    struct light_work *work = LIGHT(task);

    work->tick = tick;
    work->pos = *pos;
    work->color.a = FLASH_ALPHA;
    work->color.r = color->r;
    /* @hack redundant: the image multiplies green and blue by 0 rather than
     * storing 0 (FMUL at 0x8c06369a and 0x8c0636a4). as stores of 0.0F,
     * which agree for finite green and blue, 37 of 54 words differ */
    work->color.g = color->g * 0.0F;
    work->color.b = color->b * 0.0F;
    /* NOLINTNEXTLINE(bugprone-integer-division) */
    work->timer = FLASH_STEPS;
    work->scale = FULL_SCALE;
    task->draw = tikal_flash_draw;
    task->destructor = NULL;
}

/* fade by more each step, and end once alpha passes 0 or the timer runs
 * out. from FLASH_ALPHA, alpha passes 0 first: on the 15th step while
 * tikal_light_pace is 1, and on the 7th while it's 2 */
static void tikal_flash_step(struct task *task)
{
    struct light_work *work = LIGHT(task);

    if (--work->timer != 0) {
        /* NOLINTBEGIN(bugprone-integer-division) */
        work->color.a -= (1.0F / FLASH_STEPS) * (1.0F / FLASH_STEPS) *
                         (FLASH_STEPS - work->timer);
        /* NOLINTEND(bugprone-integer-division) */
        if (work->color.a < 0.0F) {
            task_destroy(task);
        } else {
            tikal_flash_draw(task);
        }
    } else {
        task_destroy(task);
    }
}

static void tikal_flash_draw(struct task *task)
{
    struct light_work *work = LIGHT(task);

    tikal_light_draw_sprite(&tikal_spark_sprite, &work->pos, &work->color, 0,
                            work->scale * SPARK_SPRITE_SCALE);
}

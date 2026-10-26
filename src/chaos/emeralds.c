/* the Chaos Emeralds that circle a Chaos and fly home */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "chaos/emeralds.h"
#include "chaos/models.h" /* NOLINT(misc-include-cleaner) */
#include "chaos/steer.h"
#include "draw/motion.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

#define EMERALD_SLOTS 7
/* each emerald's sprites: three glints, one after another */
#define EMERALD_GLINTS 3
/* the steps each glint shows for */
#define GLINT_STEPS 3
/* an emerald grows, and shrinks, by 1 / SCALE_PARTS of its full scale a
 * step. SHC folds 1.0F / SCALE_PARTS to 0x3d888888 (0x8c0fd8c8), the float
 * just below 1/15, so the grow takes 16 steps, to about 16/15 of the full
 * scale, and the shrink from there 16 */
#define SCALE_PARTS 15.0F
/* how far an emerald flies each step. SHC folds 0.3F * 3.0F to the float
 * just below 0.9F, which 0.9F would not give */
#define EMERALD_SPEED (0.3F * 3.0F)

enum emerald_mode {
    EMERALD_INIT,
    EMERALD_GROW, /* where it starts */
    EMERALD_CIRCLE, /* round the boss, until chaos_emerald_return_all */
    EMERALD_RETURN, /* to where it started */
    EMERALD_SHRINK /* there, then end */
};

struct emerald_vars {
    Float scale;
    Sint16 tick; /* the steps its glint has shown for */
    Sint16 glint; /* which of its glints shows */
    /* its first sprite's index: its slot times EMERALD_GLINTS */
    /* @hack type: an array of one, which SHC reads through the member's
     * address (MOV R14,R5 and ADD #16,R5 at 0x8c0fd74c in
     * chaos_emerald_draw); as a plain Sint8, chaos_emerald_step runs 2 bytes
     * short and, summed over the three functions that use it, 152 of 442
     * words differ */
    Sint8 sprite[1];
};

#define VARS(work) ((struct emerald_vars *)&(work)->vars)

/* an emerald task's extra */
struct emerald {
    Angle ang; /* its angle round the boss */
    Float radius;
    Float height;
    Float bob; /* how far it rises and falls */
    Angle bob_ang;
    NJS_POINT3 *home;
};

/* the count of the emeralds' texanims, which nothing reads */
/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const Sint32 chaos_emerald_texanim_count =
    EMERALD_SLOTS * EMERALD_GLINTS;

static NJS_TEXANIM chaos_emerald_texanims[EMERALD_SLOTS * EMERALD_GLINTS] = {
#include "chaos/emeralds/chaos_emerald_texanims.inc"
};
static NJS_SPRITE chaos_emerald_sprite = {
#include "chaos/emeralds/chaos_emerald_sprite.inc"
};
static NJS_ARGB chaos_emerald_color = {0.0F, 0.0F, 0.0F, 0.0F};

static struct task *chaos_emerald_slots[EMERALD_SLOTS];

/* on a frame's first step, draw the emerald's glint, added over what's
 * behind it */
static void chaos_emerald_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step) {
        return;
    }
    chaos_emerald_color.a = 0.5F;
    njSetConstantMaterial(&chaos_emerald_color);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    chaos_emerald_sprite.sx = chaos_emerald_sprite.sy = VARS(work)->scale;
    njDrawSprite3D(&chaos_emerald_sprite,
                   VARS(work)->glint + VARS(work)->sprite[0],
                   NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE);
    njPopMatrix(1);
}

/* each step, the emerald flies EMERALD_SPEED toward where its mode takes
 * it, or all the way when that is nearer. then it draws itself */
static void chaos_emerald_step(struct task *task)
{
    struct task_work *work = task->work;
    struct emerald *emerald = task->extra;
    NJS_POINT3 to;
    NJS_POINT3 way;
    Float distance;
    Float scale;

    switch (work->mode) {
    case EMERALD_INIT:
        /* @hack type: the tick and the glint cleared in one store,
         * MOV.L R11,@(12,R14) at 0x8c0fd7c0; as two, the function runs 4
         * bytes long and 92 of 326 words differ */
        work->vars.l[1] = 0;
        VARS(work)->scale = 0.0F;
        way = *emerald->home;
        njSubVector(&way, &boss_work->pos);
        emerald->ang = (Angle)(atan2f(way.z, way.x) * ANGLE_PER_RADIAN);
        emerald->radius = njSqrt((way.x * way.x) + (way.z * way.z));
        emerald->height = work->pos.y + chaos_params->orbit_height;
        emerald->bob = way.y * 0.2F;
        emerald->bob_ang = 0;
        task->draw = chaos_emerald_draw;
        work->mode = EMERALD_GROW;
        /* fall through */
    case EMERALD_GROW:
        to = *emerald->home;
        VARS(work)->scale += 1.0F / SCALE_PARTS;
        if (VARS(work)->scale >= 1.0F) {
            work->mode = EMERALD_CIRCLE;
        }
        break;
    case EMERALD_CIRCLE:
        VARS(work)->tick++;
        if (VARS(work)->tick >= GLINT_STEPS) {
            VARS(work)->tick = 0;
            VARS(work)->glint++;
            if (VARS(work)->glint >= EMERALD_GLINTS) {
                VARS(work)->glint = 0;
            }
        }
        emerald->ang += NJM_DEG_ANG(1);
        to.x = boss_work->pos.x + (njCos(emerald->ang) * emerald->radius);
        to.z = boss_work->pos.z + (njSin(emerald->ang) * emerald->radius);
        emerald->radius = emerald->radius + 1.0F < chaos_params->orbit_radius
                              ? emerald->radius + 1.0F
                              : chaos_params->orbit_radius;
        to.y = emerald->height + (emerald->bob * njSin(emerald->bob_ang));
        emerald->bob_ang += 0xc00;
        break;
    case EMERALD_RETURN:
        to = *emerald->home;
        /* @hack spelling: the distance through a local, which copies
         * njDistanceP2P's result to FR4 (FMOV.S FR0,FR4 at 0x8c0fd994); in
         * the test, it is compared in FR0, emeralds.c's code runs 32 bytes
         * short and 127 of 326 words differ */
        distance = njDistanceP2P(&to, &work->pos);
        if (distance < 2.0F * EMERALD_SPEED) {
            work->mode = EMERALD_SHRINK;
        }
        break;
    case EMERALD_SHRINK:
        to = *emerald->home;
        VARS(work)->scale -= 1.0F / SCALE_PARTS;
        if (VARS(work)->scale <= 0.0F) {
            chaos_emerald_slots[VARS(work)->sprite[0] / EMERALD_GLINTS] = NULL;
            task_kill(task);
            return;
        }
        break;
    }
    way = to;
    njSubVector(&way, &work->pos);
    scale = EMERALD_SPEED /
            (njScalor(&way) < EMERALD_SPEED ? EMERALD_SPEED : njScalor(&way));
    way.x *= scale;
    way.y *= scale;
    way.z *= scale;
    njAddVector(&work->pos, &way);
    chaos_emerald_draw(task);
}

void chaos_emerald_clear(void)
{
    int i;

    for (i = 0; i < EMERALD_SLOTS; i++) {
        chaos_emerald_slots[i] = NULL;
    }
}

struct task *chaos_emerald_create(NJS_POINT3 *home, int slot)
{
    struct task *task;
    struct task_work *work;
    struct emerald *emerald;

    if (chaos_emerald_slots[slot] != NULL) {
        task_destroy(chaos_emerald_slots[slot]);
    }
    task = task_create(TASK_WORK | TASK_MOTION, 2, chaos_emerald_step);
    if (task != NULL) {
        work = task->work;
        task->extra = heap_alloc(sizeof(struct emerald));
        work->pos = *home;
        VARS(work)->sprite[0] = slot * EMERALD_GLINTS;
        emerald = task->extra;
        emerald->home = home;
        chaos_emerald_slots[slot] = task;
    }
    return task;
}

void chaos_emerald_return_all(void)
{
    struct task *task;
    int i;

    for (i = 0; i < EMERALD_SLOTS; i++) {
        task = chaos_emerald_slots[i];
        if (task != NULL && task->work->mode != EMERALD_SHRINK) {
            task->work->mode = EMERALD_RETURN;
        }
    }
}

/* O ICEBAR2: O ICEBAR1's row of ice blocks, which rises from the water
 * while switch 1 is on */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "effect/dust.h"
#include "effect/snow.h"
#include "object/switch.h"
#include "stg08/ice_bar_2.h"
#include "stg08/splash.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL ice_bar_block_model;
extern NJS_MODEL ice_bar_chip_model;
extern NJS_TEXLIST alpha_water_texlist;

#define ICE_BAR_SWITCH 1

/* a bar has 1 to ICE_BAR_BLOCKS blocks, BLOCK_SPACING apart along its x, and
 * a pool of ICE_BAR_CHIPS chips */
#define ICE_BAR_BLOCKS 50
#define BLOCK_SPACING 30.0F
#define ICE_BAR_CHIPS 300

/* as the bar rises, a splash comes up on each odd step until the splashes
 * reach its end. each lies SPLASH_SPACING along the bar and turns SPLASH_TURN
 * for each step of the rise. from step BLOCKS_START, the next block rises on
 * each even step */
#define SPLASH_SPACING 15.0F
#define SPLASH_TURN 0x2000
#define SPLASH_SIZE 4.0F
#define BLOCKS_START 8

/* a block starts to shake once touched, or once it has stood BLOCK_STANDS
 * steps. its neighbors start once it has shaken more than BLOCK_SPREADS */
#define BLOCK_STANDS 900
#define BLOCK_SPREADS 130
/* a shaking block throws off chips until step BLOCK_CHIPS_END of its shake.
 * until BLOCK_SINK_END it gives off mist and sinks BLOCK_SINK a step. it
 * shakes until BLOCK_SHAKE_END, then falls away, tumbling, and is gone at
 * BLOCK_FALL_END */
#define BLOCK_CHIPS_END 40
#define BLOCK_SINK_END 100
#define BLOCK_SINK 0.02F
#define BLOCK_SHAKE_END 150
#define BLOCK_FALL_END 200
/* the speed a falling block gains downward each step */
#define BLOCK_FALL 0.2F

/* the steps a chip flies, shrinking to nothing */
#define CHIP_LIFE 60
/* a chip gains CHIP_GRAVITY downward each step. it keeps CHIP_DRAG of its
 * speed and CHIP_TURN_DRAG of its turn each step */
#define CHIP_GRAVITY 0.10888889F
#define CHIP_DRAG 0.86F
#define CHIP_TURN_DRAG 0.97F

/* a block's glint draws only where the block's position has a z of GLINT_Z
 * or more. the position is taken through the matrix the draw starts with */
#define GLINT_Z 100.0F

/* random Floats as RANDOM gives them. RANDOM_SIGNED is from -1 up to 1, and
 * RANDOM_NEAR from mid - half up to mid + half */
#define RANDOM_SIGNED() ((rand() * (2 * RAND_SCALE)) - 1.0F)
#define RANDOM_NEAR(mid, half)                                                 \
    ((rand() * ((2 * (half)) * RAND_SCALE)) + ((mid) - (half)))

/* a chip of ice that a shaking block throws off, from its bar's pool */
struct ice_bar_2_chip {
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    struct angles ang;
    struct angles turn; /* added to ang each step */
    Float scale;
    struct ice_bar_2_chip *next;
    Uint16 life; /* steps flown */
};

/* a bar's chip pool, from the heap. flying and free are lists linked through
 * each chip's next */
struct ice_bar_2_chips {
    struct ice_bar_2_chip *flying;
    struct ice_bar_2_chip *free;
    struct ice_bar_2_chip chips[ICE_BAR_CHIPS];
};

struct ice_bar_2_vars {
    /* from the heap, a slot for each block, null until the block rises */
    struct task **blocks;
    struct ice_bar_2_chips *chips;
    Sint16 count; /* ICE_BAR_COUNT, held to 1 to ICE_BAR_BLOCKS */
};

#define VARS(work) ((struct ice_bar_2_vars *)&(work)->vars)

enum ice_bar_2_mode { ICE_BAR_INIT, ICE_BAR_RUN };
#define ICE_BAR_RISING(work) ((work)->submode)
/* the block count, which the placement sets in scale x */
#define ICE_BAR_COUNT(work) ((work)->scl.x)

/* a falling block's motion, from the heap. the block falls by speed.y
 * alone */
struct ice_bar_2_fall {
    NJS_VECTOR speed;
    struct angles ang;
    struct angles turn; /* added to ang each step */
};

struct ice_bar_2_block_vars {
    Uint16 age; /* the steps since it rose */
    Uint16 shake; /* its shake's phase, an angle */
    Uint8 unk_04[4];
    struct ice_bar_2_fall *fall;
};

#define BLOCK(work) ((struct ice_bar_2_block_vars *)&(work)->vars)

enum ice_bar_2_block_mode { BLOCK_INIT, BLOCK_RUN, BLOCK_GONE };
#define BLOCK_SHAKING(work) ((work)->submode)
/* the steps the block has stood, then the steps it has shaken */
#define BLOCK_TIMER(work) TASK_TIMER(work)

static void ice_bar_2_init(struct task *task);
static void ice_bar_2_run(struct task *task);
static void ice_bar_2_reset(struct task_work *work);
static void ice_bar_2_draw(struct task *task);
static void ice_bar_2_alloc(struct task *task);
static void ice_bar_2_destroy(struct task *task);
static void ice_bar_2_block_step(struct task *task);
static void ice_bar_2_block_init(struct task *task);
static void ice_bar_2_block_run(struct task *task);
static void ice_bar_2_block_draw(struct task *task);
static void ice_bar_2_block_destroy(struct task *task);
static void ice_bar_2_block_shake(struct task *task);
static void ice_bar_2_preview(struct task *task);
static void ice_bar_2_chips_draw(struct task *task);
static void ice_bar_2_chip_add(struct task *task, NJS_POINT3 *pos,
                               NJS_VECTOR *speed, Float scale);
static void ice_bar_2_chips_move(struct task *task);
static void ice_bar_2_chips_reset(struct task *task);

static NJS_TEXANIM ice_bar_2_glint_texanims[8] = {
#include "stg08/ice_bar_2/ice_bar_2_glint_texanims.inc"
};
static NJS_SPRITE ice_bar_2_glint_sprite = {
#include "stg08/ice_bar_2/ice_bar_2_glint_sprite.inc"
};

/* a box about each block, 30 wide, 20 high and 28 deep. it pushes every
 * group, and none can push it. each block sets the box's z angle to its own
 * before collision_init copies the box */
/* clang-format off */
static struct collision_shape ice_bar_2_block_shape[1] = {
    {SHAPE_TURN_YZX | SHAPE_RECTANGLE2, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, -10.0F, -0.7F}, 15.0F, 10.0F, 14.0F, {0}, 0, NULL},
};
/* clang-format on */

/* a shaking block's mist. each puff writes its pos, speed and scale here
 * before snow_create_from makes it */
static struct dust_params ice_bar_2_mist = {
    1.0F,
    0.07F,
    0.2F,
    0.2F,
    -0.15F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.35F, 1.0F, 1.0F, 1.0F},
};

/* the origin, as njCalcPoint takes it to find a splash's place and a
 * glint's */
static NJS_POINT3 ice_bar_2_splash_origin = {0.0F, 0.0F, 0.0F};
static NJS_POINT3 ice_bar_2_glint_origin = {0.0F, 0.0F, 0.0F};

/* while the set editor holds the bar, each step makes the bar again, with
 * every block standing. each step labels the editor's angle rows and its
 * scale x row too */
void ice_bar_2_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        switch (work->mode) {
        case ICE_BAR_INIT:
            ice_bar_2_init(task);
            break;
        case ICE_BAR_RUN:
            ice_bar_2_chips_draw(task);
            ice_bar_2_run(task);
            break;
        }
    } else {
        switch (work->mode) {
        case ICE_BAR_INIT:
            ice_bar_2_init(task);
            break;
        case ICE_BAR_RUN:
            ice_bar_2_destroy(task);
            ice_bar_2_alloc(task);
            ice_bar_2_preview(task);
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> SPIN ANGLE.");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROUND ANGLE.");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> UP-DOWN ANGLE.");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> BLOCKS = %d.",
                        VARS(work)->count);
            break;
        }
    }
}

static void ice_bar_2_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = ice_bar_2_draw;
    task->destructor = ice_bar_2_destroy;
    ice_bar_2_alloc(task);
    work->mode = ICE_BAR_RUN;
}

/* while switch 1 is on, the bar rises: a splash runs along it, and the
 * blocks rise behind. once every block is gone, the bar turns the switch off.
 * with the switch off, the bar destroys its blocks and can rise again */
static void ice_bar_2_run(struct task *task)
{
    struct task_work *work = task->work;
    int count = VARS(work)->count;
    int i;
    struct task **blocks;
    NJS_POINT3 pos;
    NJS_POINT3 offset;

    if (switch_is_on(ICE_BAR_SWITCH)) {
        if (!ICE_BAR_RISING(work)) {
            ICE_BAR_RISING(work) = TRUE;
            work->timer = 0;
        }
    } else if (ICE_BAR_RISING(work)) {
        ice_bar_2_reset(work);
    }
    if (ICE_BAR_RISING(work) == TRUE) {
        if (TASK_TIMER(work) <= count * 2 && (TASK_TIMER(work) & 1)) {
            Angle ang;

            njPushMatrix(&_unit_matrix_);
            njTranslateV(NULL, &work->pos);
            TURN_VIA(njRotateY, work->ang.y, ang);
            TURN_VIA(njRotateZ, work->ang.z, ang);
            TURN_VIA(njRotateX, work->ang.x, ang);
            njTranslate(NULL, TASK_TIMER(work) * SPLASH_SPACING, 0.0F, 0.0F);
            njCalcPoint(NULL, &ice_bar_2_splash_origin, &pos);
            njPopMatrix(1);
            icecap_splash_create(&pos, TASK_TIMER(work) * SPLASH_TURN,
                                 SPLASH_SIZE);
        }
        if (TASK_TIMER(work) >= BLOCKS_START) {
            struct task **slot;

            i = (TASK_TIMER(work) - BLOCKS_START) / 2;
            if (i < count) {
                /* @hack spelling: the slot taken through a copy of the slots'
                 * base, which loads the base into R4 after i's copy
                 * (0x0c9044ac, 0x0c9044ae); from VARS(work)->blocks
                 * directly, R1 takes it first and 3 of 301 words differ */
                struct task **slots = VARS(work)->blocks;

                if (*(slot = &slots[i]) == NULL) {
                    Angle ang;
                    struct task *block = task_create_child(
                        TASK_WORK, ice_bar_2_block_step, task);

                    njPushMatrix(&_unit_matrix_);
                    njTranslateV(NULL, &work->pos);
                    TURN_VIA(njRotateY, work->ang.y, ang);
                    TURN_VIA(njRotateZ, work->ang.z, ang);
                    TURN_VIA(njRotateX, work->ang.x, ang);
                    offset.x = i * BLOCK_SPACING;
                    offset.y = 0.0F;
                    offset.z = 0.0F;
                    njCalcPoint(NULL, &offset, &block->work->pos);
                    block->work->ang = work->ang;
                    njPopMatrix(1);
                    *slot = block;
                }
            }
        }
        if (TASK_TIMER(work) < (count * 2) + 50) {
            /* @hack spelling: the increment written out, which keeps task
             * in R9 and count * 2 in R10 (0x0c90435e, 0x0c9043f6);
             * as work->timer++, the two trade and 14 of 301 words differ */
            work->timer = work->timer + 1;
        }
        blocks = VARS(work)->blocks;
        /* @hack spelling: the increment written out, which keeps task in
         * R9 and count * 2 in R10 (0x0c90435e, 0x0c9043f6); as
         * blocks++, the two trade and 14 of 301 words differ */
        for (i = 0; i < count; i++, blocks = blocks + 1) {
            struct task *block = *blocks;
            struct task *other;
            struct task_work *beside;

            if (block != NULL) {
                if (i < count - 1) {
                    other = blocks[1];
                    if (other != NULL) {
                        beside = other->work;
                        if (BLOCK_SHAKING(beside) &&
                            BLOCK_TIMER(beside) > BLOCK_SPREADS) {
                            ice_bar_2_block_shake(block);
                        }
                    }
                }
                if (i > 0) {
                    other = blocks[-1];
                    if (other != NULL) {
                        beside = other->work;
                        if (BLOCK_SHAKING(beside) &&
                            BLOCK_TIMER(beside) > BLOCK_SPREADS) {
                            ice_bar_2_block_shake(block);
                        }
                    }
                }
            }
        }
        blocks = VARS(work)->blocks;
        /* @hack spelling: the increment written out, which keeps task in
         * R9 and count * 2 in R10 (0x0c90435e, 0x0c9043f6); as i++,
         * the two trade and 14 of 301 words differ */
        for (i = 0; i < count; i = i + 1) {
            struct task *block = *blocks++;

            if (block == NULL || block->work->mode != BLOCK_GONE) {
                break;
            }
        }
        if (i >= count) {
            switch_set(ICE_BAR_SWITCH, FALSE);
        }
    }
    ice_bar_2_chips_move(task);
    task_step_children(task);
}

static void ice_bar_2_reset(struct task_work *work)
{
    struct task **blocks = VARS(work)->blocks;
    int count = VARS(work)->count;
    int i;

    for (i = 0; i < count; i++, blocks++) {
        if (*blocks != NULL) {
            task_destroy(*blocks);
            *blocks = NULL;
        }
    }
    ICE_BAR_RISING(work) = FALSE;
}

static void ice_bar_2_draw(struct task *task)
{
    ice_bar_2_chips_draw(task);
}

/* allocate the bar's block slots, ICE_BAR_COUNT of them held to 1 to
 * ICE_BAR_BLOCKS, and its chip pool. neither allocation is tested for null */
static void ice_bar_2_alloc(struct task *task)
{
    struct task_work *work = task->work;
    int count;
    int i;
    struct task **blocks;

    ICE_BAR_RISING(work) = FALSE;
    count = ICE_BAR_COUNT(work);
    if (count < 1) {
        count = 1;
    } else if (count > ICE_BAR_BLOCKS) {
        count = ICE_BAR_BLOCKS;
    }
    VARS(work)->count = count;
    if (count) {
        blocks = (struct task **)heap_alloc(count * sizeof(struct task *));
        VARS(work)->blocks = blocks;
        for (i = 0; i < count; i++) {
            *blocks++ = NULL;
        }
    } else {
        VARS(work)->blocks = NULL;
    }
    VARS(work)->chips = heap_alloc(sizeof(struct ice_bar_2_chips));
    ice_bar_2_chips_reset(task);
    task_step_children(task);
}

static void ice_bar_2_destroy(struct task *task)
{
    task_destroy_children(task);
    heap_free((void *)VARS(task->work)->blocks);
    heap_free(VARS(task->work)->chips);
}

static void ice_bar_2_block_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case BLOCK_INIT:
        ice_bar_2_block_init(task);
        break;
    case BLOCK_RUN:
        ice_bar_2_block_draw(task);
        ice_bar_2_block_run(task);
        break;
    case BLOCK_GONE:
        break;
    }
}

/* allocate the block's fall and, unless the set editor holds the bar, give
 * the block its box. a failed allocation prints "Memory shortage" and leaves
 * the block without its draw, destructor or box */
static void ice_bar_2_block_init(struct task *task)
{
    struct task_work *work = task->work;
    struct ice_bar_2_fall *fall;

    work->mode = BLOCK_RUN;
    BLOCK_SHAKING(work) = FALSE;
    work->timer = 0;
    BLOCK(work)->age = 0;
    BLOCK(work)->shake = 0;
    if ((fall = heap_alloc(sizeof(struct ice_bar_2_fall))) == NULL) {
        njPrintC(NJM_LOCATION(3, 7), "Memory shortage");
        return;
    }
    BLOCK(work)->fall = fall;
    fall->speed.x = 0.0F;
    fall->speed.y = 0.0F;
    fall->speed.z = 0.0F;
    fall->ang.x = 0;
    fall->ang.y = 0;
    fall->ang.z = 0;
    task->draw = ice_bar_2_block_draw;
    task->destructor = ice_bar_2_block_destroy;
    if (!placement_editing(task->parent)) {
        ice_bar_2_block_shape[0].ang.z = work->ang.z;
        collision_init(task, ice_bar_2_block_shape, COLLISION_OBJECT, 1, 1);
    }
}

/* a block stands until touched or for BLOCK_STANDS steps, then shakes, sinks
 * and falls away, tumbling. while the set editor holds the bar, the block only
 * ages */
static void ice_bar_2_block_run(struct task *task)
{
    struct task_work *work = task->work;
    struct ice_bar_2_fall *fall = BLOCK(work)->fall;
    NJS_VECTOR dir;
    NJS_VECTOR speed;
    NJS_POINT3 offset;
    NJS_POINT3 pos;
    Angle ang;
    Float edge;
    Float rand_scale;

    if (!placement_editing(task->parent)) {
        if (!BLOCK_SHAKING(work)) {
            work->timer++;
            /* @bug memory: a block has no collision record when its fall
             * couldn't be allocated, or when it started while the set editor
             * held the bar. ice_bar_2_preview leaves such blocks as the bar's
             * children, and once the editor lets go, ice_bar_2_run steps them
             * here. this reads the record's flags through null, at
             * 0x0c90491a */
            if ((work->collision->flags & COLLISION_HIT) ||
                BLOCK_TIMER(work) >= BLOCK_STANDS) {
                ice_bar_2_block_shake(task);
            }
        } else {
            /* @hack spelling: 15.0F through edge, which puts it first in
             * the pool, at 0x0c904c60, and in FR12 (0x0c904964); as the
             * constant at each use, it and RAND_SCALE trade places and 2 of
             * 615 words differ */
            /* @hack spelling: edge set before rand_scale; the other way round,
             * the two trade places in the pool and in FR12 and FR13, and 2 of
             * 615 words differ */
            edge = 15.0F;
            /* @hack spelling: RAND_SCALE through rand_scale, which puts it in
             * the pool at 0x0c904c64, ahead of njCalcPoint's address
             * (0x0c904976); as the constant at each use, the two trade and
             * 2 of 615 words differ */
            rand_scale = RAND_SCALE;
            if (BLOCK_TIMER(work) < BLOCK_CHIPS_END) {
                njPushMatrix(&_unit_matrix_);
                njTranslateV(NULL, &work->pos);
                TURN_VIA(njRotateY, work->ang.y, ang);
                TURN_VIA(njRotateZ, work->ang.z, ang);
                TURN_VIA(njRotateX, work->ang.x, ang);
                dir.x = RANDOM(-0.3F, 0.3F);
                dir.y = RANDOM(-0.5F, 0.2F);
                dir.z = RANDOM_NEAR(0.3F, 0.25F);
                offset.x = edge;
                offset.y = (RANDOM_SIGNED() * 8.0F) - 3.0F;
                offset.z = 8.0F;
                njCalcPoint(NULL, &offset, &pos);
                njCalcVector(NULL, &dir, &speed);
                ice_bar_2_chip_add(task->parent, &pos, &speed,
                                   (rand() * rand_scale) + 1.0F);
                dir.x = RANDOM(-0.3F, 0.3F);
                dir.y = RANDOM(-0.5F, 0.2F);
                dir.z = RANDOM_NEAR(-0.3F, 0.25F);
                offset.x = edge;
                offset.y = (RANDOM_SIGNED() * 8.0F) - 3.0F;
                offset.z = -8.0F;
                njCalcPoint(NULL, &offset, &pos);
                njCalcVector(NULL, &dir, &speed);
                ice_bar_2_chip_add(task->parent, &pos, &speed,
                                   (rand() * rand_scale) + 1.0F);
                njPopMatrix(1);
            }
            if (BLOCK_TIMER(work) < BLOCK_SINK_END) {
                njPushMatrix(&_unit_matrix_);
                njTranslateV(NULL, &work->pos);
                TURN_VIA(njRotateY, work->ang.y, ang);
                TURN_VIA(njRotateZ, work->ang.z, ang);
                TURN_VIA(njRotateX, work->ang.x, ang);
                if (rand() * rand_scale < 0.05F) {
                    dir.x = RANDOM_SIGNED() * 3.0F;
                    dir.y = RANDOM_SIGNED() * 2.0F;
                    dir.z = RANDOM_SIGNED() * 3.0F;
                    offset.x = edge;
                    offset.y = 0.0F;
                    offset.z = RANDOM_SIGNED() * 10.0F;
                    njCalcPoint(NULL, &offset, &pos);
                    njCalcVector(NULL, &dir, &speed);
                    ice_bar_2_mist.pos = pos;
                    ice_bar_2_mist.speed = speed;
                    ice_bar_2_mist.scale = 0.04F;
                    snow_create_from(&ice_bar_2_mist);
                }
                if (rand() * rand_scale < 0.05F) {
                    dir.x = RANDOM_SIGNED() * 3.0F;
                    dir.y = RANDOM_SIGNED() * 2.0F;
                    dir.z = RANDOM_SIGNED() * 3.0F;
                    offset.x = -15.0F;
                    offset.y = 0.0F;
                    offset.z = RANDOM_SIGNED() * 10.0F;
                    njCalcPoint(NULL, &offset, &pos);
                    njCalcVector(NULL, &dir, &speed);
                    ice_bar_2_mist.pos = pos;
                    ice_bar_2_mist.speed = speed;
                    ice_bar_2_mist.scale = 0.04F;
                    snow_create_from(&ice_bar_2_mist);
                }
                njPopMatrix(1);
                BLOCK(work)->shake += (int)RANDOM(6600.0F, 20600.0F);
            } else if (BLOCK_TIMER(work) < BLOCK_SHAKE_END) {
                BLOCK(work)->shake += (int)RANDOM(6600.0F, 20600.0F);
            } else if (BLOCK_TIMER(work) == BLOCK_SHAKE_END) {
                work->pos.y -= BLOCK_SINK_END * BLOCK_SINK;
                fall->turn.x = RANDOM(-512.0F, 512.0F);
                fall->turn.y = RANDOM(-512.0F, 512.0F);
                fall->turn.z = -(rand() * (512.0F * RAND_SCALE));
            } else if (BLOCK_TIMER(work) < BLOCK_FALL_END) {
                work->pos.y += fall->speed.y;
                fall->speed.y -= BLOCK_FALL;
                fall->ang.x += fall->turn.x;
                fall->ang.y += fall->turn.y;
                fall->ang.z += fall->turn.z;
            } else {
                work->mode = BLOCK_GONE;
            }
            work->timer++;
        }
        collision_register(work);
    }
    BLOCK(work)->age++;
}

/* draw the block and its glint, on a frame's first step only */
static void ice_bar_2_block_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct ice_bar_2_fall *fall = BLOCK(work)->fall;
    Angle ang;
    NJS_POINT3 pos;
    NJS_ARGB color;

    if (frame_step > 0) {
        return;
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    if (BLOCK_SHAKING(work)) {
        if (BLOCK_TIMER(work) < BLOCK_SINK_END) {
            njTranslate(NULL, 0.0F,
                        -(BLOCK_TIMER(work) * BLOCK_SINK) +
                            (njCos(BLOCK(work)->shake) * 0.33F),
                        0.0F);
        } else if (BLOCK_TIMER(work) < BLOCK_SHAKE_END) {
            njTranslate(NULL, 0.0F, -(BLOCK_SINK_END * BLOCK_SINK),
                        njCos(BLOCK(work)->shake) * 0.4F);
        } else {
            TURN_VIA(njRotateY, fall->ang.y, ang);
            TURN_VIA(njRotateZ, fall->ang.z, ang);
            TURN_VIA(njRotateX, fall->ang.x, ang);
        }
    }
    texture_set_stage();
    draw_model(&ice_bar_block_model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njCalcPoint(NULL, &ice_bar_2_glint_origin, &pos);
    if (pos.z >= GLINT_Z) {
        TURN_VIA(njRotateY, camera_work->ang.y, ang);
        TURN_VIA(njRotateX, camera_work->ang.x, ang);
        TURN_VIA(njRotateZ, -BLOCK(work)->age * 512, ang);
        njScale(NULL, 4.0F, 4.0F, 4.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        texture_set_stage();
        color.a = 0.2F;
        color.r = 1.0F;
        color.g = 1.0F;
        color.b = 1.0F;
        njSetConstantMaterial(&color);
        njDrawSprite3D(&ice_bar_2_glint_sprite, (BLOCK(work)->age / 4) & 7,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR | NJD_SPRITE_ANGLE);
        color.a = 0.0F;
        color.r = 0.0F;
        color.g = 0.0F;
        color.b = 0.0F;
        njSetConstantMaterial(&color);
    }
    njPopMatrix(1);
}

static void ice_bar_2_block_destroy(struct task *task)
{
    heap_free(BLOCK(task->work)->fall);
}

static void ice_bar_2_block_shake(struct task *task)
{
    struct task_work *work = task->work;

    if (!BLOCK_SHAKING(work)) {
        BLOCK_SHAKING(work) = TRUE;
        work->timer = 0;
    }
}

/* for the set editor, create every block at once. step them twice, once to
 * start each and once to draw it */
static void ice_bar_2_preview(struct task *task)
{
    struct task_work *work = task->work;
    int count;
    struct task *block;
    NJS_POINT3 offset;
    Angle ang;
    int i;

    offset.y = 0.0F;
    offset.z = 0.0F;
    count = VARS(work)->count;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    for (i = 0; i < count; i++) {
        block = task_create_child(TASK_WORK, ice_bar_2_block_step, task);
        offset.x = i * BLOCK_SPACING;
        njCalcPoint(NULL, &offset, &block->work->pos);
        block->work->ang = work->ang;
    }
    njPopMatrix(1);
    task_step_children(task);
    task_step_children(task);
}

/* draw the flying chips, on a frame's first step only. each shrinks by the
 * share of CHIP_LIFE it has flown */
static void ice_bar_2_chips_draw(struct task *task)
{
    struct ice_bar_2_chips *chips = VARS(task->work)->chips;
    struct ice_bar_2_chip *chip;
    Float size;
    Angle ang;

    if (frame_step > 0) {
        return;
    }
    for (chip = chips->flying; chip != NULL; chip = chip->next) {
        size = chip->scale * (1.0F - (chip->life * (1.0F / CHIP_LIFE)));
        njPushMatrix(NULL);
        njTranslateV(NULL, &chip->pos);
        TURN_VIA(njRotateY, chip->ang.y, ang);
        TURN_VIA(njRotateX, chip->ang.x, ang);
        TURN_VIA(njRotateZ, chip->ang.z, ang);
        njScale(NULL, size, size, size);
        draw_model(&ice_bar_chip_model);
        njPopMatrix(1);
    }
}

/* throw a chip from pos at speed, of size scale, at a random angle and turn.
 * nothing when the pool has no free chip */
static void ice_bar_2_chip_add(struct task *task, NJS_POINT3 *pos,
                               NJS_VECTOR *speed, Float scale)
{
    struct ice_bar_2_chips *chips = VARS(task->work)->chips;
    struct ice_bar_2_chip *chip = chips->free;

    if (chip != NULL) {
        chips->free = chip->next;
        chip->next = chips->flying;
        chips->flying = chip;
        chip->pos = *pos;
        chip->speed = *speed;
        chip->ang.x = RANDOM_ANGLE();
        chip->ang.y = RANDOM_ANGLE();
        chip->ang.z = RANDOM_ANGLE();
        chip->turn.x = RANDOM_SIGNED() * 8192.0F;
        chip->turn.y = RANDOM_SIGNED() * 8192.0F;
        chip->turn.z = RANDOM_SIGNED() * 8192.0F;
        chip->scale = scale;
        chip->life = 0;
    }
}

static void ice_bar_2_chips_move(struct task *task)
{
    struct ice_bar_2_chips *chips = VARS(task->work)->chips;
    struct ice_bar_2_chip *chip = chips->flying;
    struct ice_bar_2_chip *prev = NULL;
    struct ice_bar_2_chip *next;

    while (chip != NULL) {
        next = chip->next;
        chip->life++;
        if (chip->life > CHIP_LIFE) {
            chip->next = chips->free;
            chips->free = chip;
            if (prev == NULL) {
                chips->flying = next;
            } else {
                prev->next = next;
            }
        } else {
            chip->pos.x += chip->speed.x;
            chip->pos.y += chip->speed.y;
            chip->pos.z += chip->speed.z;
            chip->speed.y -= CHIP_GRAVITY;
            chip->speed.x += 0.1F - (rand() * (0.2F * RAND_SCALE));
            chip->speed.z += 0.1F - (rand() * (0.2F * RAND_SCALE));
            chip->speed.x *= CHIP_DRAG;
            chip->speed.y *= CHIP_DRAG;
            chip->speed.z *= CHIP_DRAG;
            chip->ang.x += chip->turn.x;
            chip->ang.y += chip->turn.y;
            chip->ang.z += chip->turn.z;
            chip->turn.x += 2048.0F - (rand() * (4096.0F * RAND_SCALE));
            chip->turn.y += 2048.0F - (rand() * (4096.0F * RAND_SCALE));
            chip->turn.z += 2048.0F - (rand() * (4096.0F * RAND_SCALE));
            chip->turn.x *= CHIP_TURN_DRAG;
            chip->turn.y *= CHIP_TURN_DRAG;
            chip->turn.z *= CHIP_TURN_DRAG;
            prev = chip;
        }
        chip = next;
    }
}

static void ice_bar_2_chips_reset(struct task *task)
{
    struct ice_bar_2_chips *chips = VARS(task->work)->chips;
    struct ice_bar_2_chip *chip;
    int i;

    chips->flying = NULL;
    chips->free = chips->chips;
    chip = chips->chips;
    for (i = 0; i < ICE_BAR_CHIPS - 1; i++) {
        chip->next = chip + 1;
        chip++;
    }
    chip->next = NULL;
}

/* print one cell of a table as "[row][column]=value". nothing calls the
 * function, so SHC leaves it out but keeps its string, at 0x0cac30ac */
/* NOLINTNEXTLINE(clang-diagnostic-unused-function) */
static void ice_bar_2_print_cell(Int loc, int row, int column, Float value)
{
    debug_print(loc, "[%d][%d]=%g", row, column, value);
}

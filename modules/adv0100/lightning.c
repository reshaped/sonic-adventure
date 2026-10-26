/* O LIGHTNING lightning: flashes in the clouds above the object, and bolts
 * that strike down from them to its height */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv0100/lightning.h"
#include "camera/camera.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* lightning_quad_model's four points, which lightning_draw_segments writes
 * again for each quad it draws */
extern NJS_POINT3 lightning_quad_points[4];
extern NJS_MODEL lightning_quad_model;
extern NJS_MODEL lightning_flash_model;

/* the bolt segments and flashes struct lightning holds */
#define SEGMENTS 50
#define FLASHES 15

/* each calm step's chance of a strike, and failing that, of a flash in the
 * clouds */
#define STRIKE_CHANCE 0.016F
#define FLASH_CHANCE 0.07F
/* the clouds' height above the object, where the flashes light and the bolts
 * start */
#define SKY_HEIGHT 2000.0F
/* how far each segment of a bolt drops, and what each adds to the bolt's
 * delay. a segment takes the delay truncated to whole steps, so with 0.6 it
 * shows 0 or 1 steps after the segment before */
#define SEGMENT_DROP 200.0F
#define SEGMENT_DELAY 0.6F
/* the chance that a bolt forks at the end of a segment that isn't its last */
#define BRANCH_CHANCE 0.2F
/* a segment's glow reaches GLOW_SCALE times its end_width to either side */
#define GLOW_SCALE 6.0F
/* how far across x and z a flash lights from where it centers. a calm sky's
 * flash centers on the object, and the flashes after a strike on the bolt's
 * end */
#define FLASH_SPREAD 1800.0F
/* how far across x and z from the object a bolt starts, and how far from
 * the bolt's start the strike's first flash lights */
#define STRIKE_SPREAD 250.0F
#define START_FLASH_SPREAD 500.0F
/* each flash after the one at a bolt's end lights 1 to this many steps after
 * it */
#define AFTER_FLASH_WAIT 4.0F

/* the clips the bolts and flashes draw with, and those lightning_draw sets
 * after them */
#define DRAW_NEAR_CLIP (-1.0F)
#define DRAW_FAR_CLIP (-65535.0F)
#define NEAR_CLIP (-3.0F)
#define FAR_CLIP (-3000.0F)

/* a straight segment of a bolt, drawn once its delay runs out */
struct bolt_segment {
    NJS_POINT3 start;
    NJS_POINT3 end;
    Float start_width;
    Float end_width;
    Sint16 delay; /* steps before the segment is drawn */
    struct bolt_segment *next;
};

/* a flash, drawn once its delay runs out, for the steps of its life */
struct bolt_flash {
    NJS_POINT3 pos;
    struct angles ang;
    Sint16 delay;
    Sint16 life;
    struct bolt_flash *next;
};

/* the lightning's record, at VARS(work)->bolts. lightning_alloc takes it from
 * the heap, and lightning_destroy frees it. segments and flashes list those in
 * use, newest first, and free_segments and free_flashes those free */
struct lightning {
    struct bolt_segment *segments;
    struct bolt_segment *free_segments;
    struct bolt_segment segment[SEGMENTS];
    struct bolt_flash *flashes;
    struct bolt_flash *free_flashes;
    struct bolt_flash flash[FLASHES];
};

struct lightning_vars {
    Uint8 unk_00[4];
    struct lightning *bolts;
};

#define VARS(work) ((struct lightning_vars *)&(work)->vars)

enum lightning_mode { LIGHTNING_INIT, LIGHTNING_RUN };

/* sky states, in work->submode */
enum lightning_sky {
    SKY_CALM, /* random flashes in the clouds, until a bolt strikes */
    SKY_STRIKE, /* a bolt, drawn a segment at a time */
    SKY_AFTER /* a pause before the sky is calm again */
};

/* the steps a strike's segments age before they clear, and those of the
 * pause after it */
#define STRIKE_STEPS 14
#define AFTER_STEPS 2

/* the x axis, which the camera's y angle turns into the direction across its
 * view */
static NJS_VECTOR lightning_side = {1.0F, 0.0F, 0.0F};

static void lightning_init(struct task *task);
static void lightning_alloc(struct task *task);
static void lightning_run(struct task *task);
static void lightning_draw(struct task *task);
static void lightning_strike(struct task *task);
static Float lightning_add_bolt(struct task *task, NJS_POINT3 *from,
                                NJS_POINT3 *dir, Float width, Float taper,
                                Float start_delay, int count);
static void lightning_draw_segments(struct task *task);
static void lightning_age_segments(struct task *task);
static void lightning_add_segment(struct task *task, NJS_POINT3 *start,
                                  NJS_POINT3 *end, Float start_width,
                                  Float end_width, int delay);
static void lightning_clear_segments(struct task *task);
static void lightning_draw_flashes(struct task *task);
static void lightning_age_flashes(struct task *task);
static void lightning_add_flash(struct task *task, NJS_POINT3 *pos,
                                struct angles *ang, int delay, int life);
static void lightning_clear_flashes(struct task *task);
static void lightning_destroy(struct task *task);

/* draw and run the lightning. while the set editor holds the task, also label
 * the editor's angle rows as rotations */
void egg_carrier_lightning_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        switch (work->mode) {
        case LIGHTNING_INIT:
            lightning_init(task);
            break;
        case LIGHTNING_RUN:
            lightning_draw(task);
            lightning_run(task);
            break;
        }
    } else {
        switch (work->mode) {
        case LIGHTNING_INIT:
            lightning_init(task);
            break;
        case LIGHTNING_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            lightning_draw(task);
            lightning_run(task);
            break;
        }
    }
}

static void lightning_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = lightning_draw;
    task->destructor = lightning_destroy;
    lightning_alloc(task);
    work->mode = LIGHTNING_RUN;
}

/* take the lightning's record from the heap, with every segment and flash
 * free, and start the sky calm */
static void lightning_alloc(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->bolts = heap_alloc(sizeof(struct lightning));
    lightning_clear_segments(task);
    lightning_clear_flashes(task);
    work->submode = SKY_CALM;
}

/* run the sky one step. a calm sky strikes a bolt, or failing that lights a
 * flash in the clouds within FLASH_SPREAD of the object. a strike's segments
 * clear after STRIKE_STEPS, and AFTER_STEPS later the sky is calm again */
static void lightning_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    struct angles ang;

    switch (work->submode) {
    case SKY_CALM:
        if (rand() * RAND_SCALE < STRIKE_CHANCE) {
            lightning_strike(task);
            work->submode = SKY_STRIKE;
            work->timer = 0;
        } else if (rand() * RAND_SCALE < FLASH_CHANCE) {
            pos.x = RANDOM(-1.0F, 1.0F) * FLASH_SPREAD;
            pos.z = RANDOM(-1.0F, 1.0F) * FLASH_SPREAD;
            pos.y = SKY_HEIGHT;
            ang.x = 0;
            ang.y = 0;
            ang.z = 0;
            lightning_add_flash(task, &pos, &ang, 0, (int)RANDOM(5.0F, 17.0F));
        }
        lightning_age_flashes(task);
        break;
    case SKY_STRIKE:
        if (TASK_TIMER(work) >= STRIKE_STEPS) {
            lightning_clear_segments(task);
            work->submode = SKY_AFTER;
            work->timer = 0;
            /* @hack spelling: a goto into the case's own call of
             * lightning_age_flashes, which keeps it apart from SKY_CALM's (BSR
             * at 0x0c9295ca, 0x0c9295a0); as an else, the two cases share
             * SKY_CALM's call, lightning_run runs 6 bytes short and 63 of 150
             * words differ */
            goto age;
        }
        work->timer++;
        lightning_age_segments(task);
    age:
        lightning_age_flashes(task);
        break;
    case SKY_AFTER:
        if (TASK_TIMER(work) >= AFTER_STEPS) {
            work->submode = SKY_CALM;
        } else {
            work->timer++;
            lightning_age_flashes(task);
        }
        break;
    }
}

/* draw the bolts and flashes with the clips at DRAW_NEAR_CLIP and
 * DRAW_FAR_CLIP, then set them to NEAR_CLIP and FAR_CLIP, whatever they were
 * before */
static void lightning_draw(struct task *task)
{
    njClipZ(DRAW_NEAR_CLIP, DRAW_FAR_CLIP);
    lightning_draw_segments(task);
    lightning_draw_flashes(task);
    njClipZ(NEAR_CLIP, FAR_CLIP);
}

/* strike a bolt from SKY_HEIGHT above the object, within STRIKE_SPREAD of it.
 * a flash lights in the clouds within START_FLASH_SPREAD of the bolt's start.
 * once the bolt has reached the object's height, a flash lights at its end.
 * one to three more light within FLASH_SPREAD of the end, 1 to
 * AFTER_FLASH_WAIT steps later */
static void lightning_strike(struct task *task)
{
    /* @hack redundant: work, read and never used, which has SHC load the last
     * flashes' life scale (MOVA at 0x0c9297f8) before it copies FR12 (FMOV
     * FR12,FR0 at 0x0c929810); without it, the two trade and 15 of 226 words
     * differ */
    struct task_work *work = task->work;
    NJS_POINT3 start;
    NJS_POINT3 dir;
    Float delay;
    int count;

    start.x = RANDOM(-1.0F, 1.0F) * STRIKE_SPREAD;
    start.z = RANDOM(-1.0F, 1.0F) * STRIKE_SPREAD;
    start.y = SKY_HEIGHT;
    dir.x = RANDOM(-1.0F, 1.0F) * 500.0F;
    dir.z = RANDOM(-1.0F, 1.0F) * 500.0F;
    dir.y = -SEGMENT_DROP;
    /* @hack redundant: two bare scopes, each with a pos and an ang of its own,
     * which put the first flash's pos at sp+12 and its ang at sp+24 (0x0c929714
     * to 0x0c92974e); with one pair for both flashes, the two trade slots and 8
     * of 226 words differ */
    {
        struct angles ang;
        NJS_POINT3 pos;

        pos.x = start.x + (RANDOM(-1.0F, 1.0F) * START_FLASH_SPREAD);
        pos.z = start.z + (RANDOM(-1.0F, 1.0F) * START_FLASH_SPREAD);
        pos.y = start.y;
        ang.x = 0;
        ang.y = 0;
        ang.z = 0;
        lightning_add_flash(task, &pos, &ang, 0, 16);
    }
    delay = lightning_add_bolt(task, &start, &dir, 8.0F, 0.6F, 6.0F, 10);
    {
        NJS_POINT3 pos;
        struct angles ang;

        ang.x = 0;
        ang.y = 0;
        ang.z = 0;
        lightning_add_flash(task, &start, &ang, (int)delay, 10);
        pos.y = start.y;
        for (count = (int)(rand() * (3.0F * RAND_SCALE)) + 1; count > 0;
             count--) {
            pos.x = start.x + (RANDOM(-1.0F, 1.0F) * FLASH_SPREAD);
            pos.z = start.z + (RANDOM(-1.0F, 1.0F) * FLASH_SPREAD);
            lightning_add_flash(
                task, &pos, &ang,
                (int)((int)delay + (rand() * (AFTER_FLASH_WAIT * RAND_SCALE)) +
                      1.0F),
                (int)RANDOM(2.0F, 7.0F));
        }
    }
}

/* add count segments of a bolt from *from. the first starts width wide and
 * takes start_delay as its delay. each segment drops SEGMENT_DROP and wanders
 * across x and z from *dir. each narrows by taper along its length, and adds
 * SEGMENT_DELAY to the delay. the end of each segment but the last forks,
 * with a chance of BRANCH_CHANCE, into a bolt of its own that narrows to
 * nothing. *from is left at the bolt's end. the result is the delay after
 * the last segment */
static Float lightning_add_bolt(struct task *task, NJS_POINT3 *from,
                                NJS_POINT3 *dir, Float width, Float taper,
                                Float start_delay, int count)
{
    Float delay;
    NJS_POINT3 here;
    NJS_POINT3 next;
    NJS_POINT3 step;
    NJS_POINT3 branch_dir;
    NJS_POINT3 branch_from;
    Float next_width;
    int branch_count;
    int i;

    here = *from;
    step = *dir;
    /* @hack spelling: a copy of start_delay, which has SHC hold RANDOM's scale
     * in FR13 and its -1 in FR14 (FMOV FR13,FR0 at 0x0c929910); with
     * start_delay counted up itself, the two trade and 28 of 253 words
     * differ */
    delay = start_delay;
    for (i = 0; i < count; i++) {
        step.x = (step.x * 0.2F) + (RANDOM(-1.0F, 1.0F) * 150.0F);
        step.z = (step.z * 0.2F) + (RANDOM(-1.0F, 1.0F) * 150.0F);
        step.y = -SEGMENT_DROP;
        next.x = here.x + step.x;
        next.z = here.z + step.z;
        next.y = here.y + step.y;
        next_width = width - taper;
        lightning_add_segment(task, &here, &next, width, next_width,
                              (int)delay);
        if (i != count - 1 && rand() * RAND_SCALE < BRANCH_CHANCE) {
            branch_dir.x = (step.x * 0.2F) + (RANDOM(-1.0F, 1.0F) * 250.0F);
            branch_dir.z = (step.z * 0.2F) + (RANDOM(-1.0F, 1.0F) * 250.0F);
            branch_dir.y = -SEGMENT_DROP;
            branch_from = next;
            branch_count =
                (int)(((Float)(count - i - 3) * rand() * RAND_SCALE) + 2.0F);
            lightning_add_bolt(task, &branch_from, &branch_dir, next_width,
                               next_width / branch_count, delay, branch_count);
        }
        delay += SEGMENT_DELAY;
        here = next;
        width = next_width;
    }
    *from = here;
    return delay;
}

/* draw each segment whose delay has run out as two quads along it, which spread
 * across the camera's view. the bolt reaches start_width to either side, and a
 * dimmer glow GLOW_SCALE times end_width. one random flicker a step sets the
 * brightness of both */
static void lightning_draw_segments(struct task *task)
{
    struct task_work *work = task->work;
    struct lightning *bolts = VARS(work)->bolts;
    struct bolt_segment *seg;
    Angle ang;
    NJS_VECTOR side;
    Float flicker;
    Float bright;
    Float dim;
    Float across_x;
    Float across_z;
    Float glow_x;
    Float glow_z;
    Float bolt_dx;
    Float bolt_dz;
    Float glow_dx;
    Float glow_dz;

    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    njCalcVector(NULL, &lightning_side, &side);
    njPopMatrix(1);
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    flicker = rand() * RAND_SCALE;
    bright = (flicker * 0.8F) - 0.8F;
    dim = (flicker * 0.4F) - 0.8F;
    across_x = side.x;
    across_z = side.z;
    glow_x = across_x * GLOW_SCALE;
    glow_z = across_z * GLOW_SCALE;
    for (seg = bolts->segments; seg != NULL; seg = seg->next) {
        if (seg->delay == 0) {
            bolt_dx = across_x * seg->start_width;
            bolt_dz = seg->start_width * across_z;
            glow_dx = glow_x * seg->end_width;
            glow_dz = seg->end_width * glow_z;
            draw_set_material(0.0F, bright, bright, bright);
            lightning_quad_points[0].x = seg->start.x + bolt_dx;
            lightning_quad_points[0].z = seg->start.z + bolt_dz;
            lightning_quad_points[0].y = seg->start.y;
            lightning_quad_points[1].x = seg->start.x - bolt_dx;
            lightning_quad_points[1].z = seg->start.z - bolt_dz;
            lightning_quad_points[1].y = seg->start.y;
            lightning_quad_points[2].x = seg->end.x + bolt_dx;
            lightning_quad_points[2].z = seg->end.z + bolt_dz;
            lightning_quad_points[2].y = seg->end.y;
            lightning_quad_points[3].x = seg->end.x - bolt_dx;
            lightning_quad_points[3].z = seg->end.z - bolt_dz;
            lightning_quad_points[3].y = seg->end.y;
            njDrawModel(&lightning_quad_model);
            draw_set_material(0.0F, dim, dim, dim);
            lightning_quad_points[0].x = seg->start.x + glow_dx;
            lightning_quad_points[0].z = seg->start.z + glow_dz;
            lightning_quad_points[0].y = seg->start.y;
            lightning_quad_points[1].x = seg->start.x - glow_dx;
            lightning_quad_points[1].z = seg->start.z - glow_dz;
            lightning_quad_points[1].y = seg->start.y;
            lightning_quad_points[2].x = seg->end.x + glow_dx;
            lightning_quad_points[2].z = seg->end.z + glow_dz;
            lightning_quad_points[2].y = seg->end.y;
            lightning_quad_points[3].x = seg->end.x - glow_dx;
            lightning_quad_points[3].z = seg->end.z - glow_dz;
            lightning_quad_points[3].y = seg->end.y;
            njDrawModel(&lightning_quad_model);
        }
    }
    /* @hack redundant: a never-run test of across_x + across_z + glow_x +
     * glow_dz, which keeps the image's stack slots and float registers for the
     * glow (0x0c929b8c to 0x0c929bfc); without it, sp+20 and sp+24 trade, the
     * glow's products come in another order and 106 of 273 words differ */
    if (0) {
        if (across_x + across_z + glow_x + glow_dz) {
        }
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPopMatrix(1);
}

/* count down the delay of each segment still waiting to be shown */
static void lightning_age_segments(struct task *task)
{
    struct task_work *work = task->work;
    struct lightning *bolts = VARS(work)->bolts;
    struct bolt_segment *seg;

    for (seg = bolts->segments; seg != NULL; seg = seg->next) {
        if (seg->delay != 0) {
            seg->delay--;
        }
    }
}

/* move a segment from the free list to the head of those in use, or add
 * nothing once none is free */
static void lightning_add_segment(struct task *task, NJS_POINT3 *start,
                                  NJS_POINT3 *end, Float start_width,
                                  Float end_width, int delay)
{
    struct lightning *bolts = VARS(task->work)->bolts;
    struct bolt_segment *seg = bolts->free_segments;

    if (seg != NULL) {
        bolts->free_segments = seg->next;
        seg->next = bolts->segments;
        bolts->segments = seg;
        seg->start = *start;
        seg->end = *end;
        seg->start_width = start_width;
        seg->end_width = end_width;
        seg->delay = delay;
    }
}

/* put every segment on the free list, and none in use */
static void lightning_clear_segments(struct task *task)
{
    struct lightning *bolts = VARS(task->work)->bolts;
    struct bolt_segment *seg;
    int i;

    bolts->segments = NULL;
    bolts->free_segments = bolts->segment;
    seg = bolts->segment;
    for (i = 0; i < SEGMENTS - 1; i++) {
        seg->next = seg + 1;
        seg++;
    }
    seg->next = NULL;
}

/* draw each flash whose delay has run out, each at a random brightness of its
 * own */
static void lightning_draw_flashes(struct task *task)
{
    struct task_work *work = task->work;
    struct lightning *bolts = VARS(work)->bolts;
    struct bolt_flash *flash;
    Angle ang;
    Float bright;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    for (flash = bolts->flashes; flash != NULL; flash = flash->next) {
        if (flash->delay == 0) {
            bright = (rand() * (0.3F * RAND_SCALE)) - 0.5F;
            draw_set_material(0.0F, bright, bright, bright);
            njPushMatrix(NULL);
            njTranslateV(NULL, &flash->pos);
            TURN_VIA(njRotateZ, flash->ang.z, ang);
            TURN_VIA(njRotateX, flash->ang.x, ang);
            TURN_VIA(njRotateY, flash->ang.y, ang);
            njDrawModel(&lightning_flash_model);
            njPopMatrix(1);
        }
    }
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* count each flash's delay down, then its life. a flash whose delay and life
 * are both 0 goes back on the free list */
static void lightning_age_flashes(struct task *task)
{
    struct task_work *work = task->work;
    struct lightning *bolts = VARS(work)->bolts;
    struct bolt_flash *flash = bolts->flashes;
    struct bolt_flash *prev = NULL;
    struct bolt_flash *next;

    while (flash != NULL) {
        next = flash->next;
        if (flash->delay != 0) {
            flash->delay--;
            prev = flash;
        } else if (flash->life != 0) {
            flash->life--;
            prev = flash;
        } else {
            flash->next = bolts->free_flashes;
            bolts->free_flashes = flash;
            if (prev == NULL) {
                bolts->flashes = next;
            } else {
                prev->next = next;
            }
        }
        flash = next;
    }
}

/* move a flash from the free list to the head of those in use, or add nothing
 * once none is free */
static void lightning_add_flash(struct task *task, NJS_POINT3 *pos,
                                struct angles *ang, int delay, int life)
{
    struct lightning *bolts = VARS(task->work)->bolts;
    struct bolt_flash *flash = bolts->free_flashes;

    if (flash != NULL) {
        bolts->free_flashes = flash->next;
        flash->next = bolts->flashes;
        bolts->flashes = flash;
        flash->pos = *pos;
        flash->ang = *ang;
        flash->delay = delay;
        flash->life = life;
    }
}

/* put every flash on the free list, and none in use */
static void lightning_clear_flashes(struct task *task)
{
    struct lightning *bolts = VARS(task->work)->bolts;
    struct bolt_flash *flash;
    int i;

    bolts->flashes = NULL;
    bolts->free_flashes = bolts->flash;
    flash = bolts->flash;
    for (i = 0; i < FLASHES - 1; i++) {
        flash->next = flash + 1;
        flash++;
    }
    flash->next = NULL;
}

static void lightning_destroy(struct task *task)
{
    /* @hack redundant: self, set to task's address and never read, which
     * keeps the call of heap_free a JSR inside a frame (0x0c92a018); without
     * it, SHC ends the function in a JMP to heap_free, and 11 of 11 words
     * differ */
    struct task **self = &task;

    heap_free(VARS(task->work)->bolts);
}

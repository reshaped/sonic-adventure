/* spikes and bubbles. O TOGE is a grid of spikes that rise out of their
 * holes and sink back, over and over. under water, O BUBBLE lets go a big
 * bubble at once and then every 601 steps. a player who touches one takes
 * a breath from it. O BUBBLES lets out a stream of small ones */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/mesh.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/helpers.h"
#include "object/models.h"
#include "object/placement.h"
#include "object/toge_bubble.h"
#include "player/player.h"
#include "system/frame.h"
#include "system/math.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/texture.h"
#include "system/util.h"

/* the grid turns about x by whole quarter turns, an angle's top two bits.
 * none or a half turn lays it on a floor or a ceiling, and a quarter turn
 * either way stands it on a wall */
#define QUARTER_TURNS 0xc000
#define FACING(work) ((work)->ang.x & QUARTER_TURNS)
#define FLAT(work) (FACING(work) == 0 || FACING(work) == NJM_DEG_ANG(180))

/* the spikes stand this far apart */
#define SPACING 15.0F
#define HALF_SPACING (SPACING / 2)

/* the grid's modes, which work->tick times. the spikes wait down and up
 * for WAIT_STEPS, and rise FULL_LIFT and sink back over MOVE_STEPS */
enum toge_mode { TOGE_DOWN, TOGE_RISE, TOGE_UP, TOGE_SINK };
#define WAIT_STEPS 60
#define MOVE_STEPS 10
#define FULL_LIFT 6.0F
#define LIFT_STEP (FULL_LIFT / MOVE_STEPS)

/* what the grid keeps in work->vars */
struct toge_vars {
    Uint8 unk_00[4];
    /* @hack type: the counts as an array, whose address toge_draw_grid
     * forms once (ADD #12,R2 at 0x8c06be02) and keeps at @R15 (0x8c06be06);
     * as two members, SHC reads each at its offset from the work, the
     * function runs 8 bytes short and 91 of 140 words differ */
    Sint16 count[2]; /* the spikes along x and along z */
    Float lift; /* how far the spikes stand out of their holes */
};

#define VARS(work) ((struct toge_vars *)&(work)->vars)

static void toge_destroy(struct task *task);
static void toge_draw(struct task *task);
static void toge_step(struct task *task);
static void toge_count(struct task_work *work);
static void toge_draw_grid(struct task_work *work);

/* the plate the spikes stand in, and the spikes, which hurt a player while
 * they're up. toge_init sizes both to the grid */
/* clang-format off */
static struct collision_shape toge_shapes[2] = {
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 0.0F, 0.0F}, 6.5F, 3.0F, 6.5F, {0}, 0, NULL},
    {SHAPE_RECTANGLE | SHAPE_OFF | SHAPE_TURN_YZX | SHAPE_HURTABLE,
     ATTACK(2) | GUARD(3) | SHAPE_HURTS_PLAYERS, 0,
     {0.0F, 5.0F, 0.0F}, 5.0F, 5.0F, 5.0F, {0}, 0, NULL},
};
/* clang-format on */

void toge_init(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_shape *shape;

    if (!placement_editing(task)) {
        toge_count(work);
        work->mode = TOGE_DOWN;
        work->tick = 0;
        work->ang.x &= QUARTER_TURNS;
        work->ang.z = 0;
        collision_init(task, toge_shapes, COLLISION_OBJECT, 1, 2);
        shape = work->collision->shapes;
        if (FLAT(work)) {
            shape->radius = VARS(work)->count[0] * HALF_SPACING;
            shape->size_y = 3.5F;
            shape->size_z = VARS(work)->count[1] * HALF_SPACING;
        } else {
            shape->radius = VARS(work)->count[0] * HALF_SPACING;
            shape->size_y = VARS(work)->count[1] * HALF_SPACING;
            shape->size_z = 3.5F;
        }
        shape = &work->collision->shapes[1];
        if (FLAT(work)) {
            shape->radius = (VARS(work)->count[0] * HALF_SPACING) - 1.0F;
            shape->size_y = 2.0F;
            shape->size_z = (VARS(work)->count[1] * HALF_SPACING) - 1.0F;
        } else {
            shape->radius = (VARS(work)->count[0] * HALF_SPACING) - 1.0F;
            shape->size_y = (VARS(work)->count[1] * HALF_SPACING) - 1.0F;
            shape->size_z = 2.0F;
        }
    } else {
        /* @hack redundant: an empty else, which loads toge_step's and
         * toge_draw's addresses into R2 and toge_destroy's into R3
         * (0x8c06bbbe to 0x8c06bbc4); without it, R2 and R3 trade those
         * addresses and 6 of 138 words differ */
    }
    task->step = toge_step;
    task->destructor = toge_destroy;
    task->draw = toge_draw;
}

static void toge_destroy(struct task *task)
{
}

static void toge_draw(struct task *task)
{
    toge_draw_grid(task->work);
}

/* raise and lower the spikes, which hurt only while they're up. while the
 * set editor holds the grid, count the spikes again instead */
static void toge_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        njPrintC(NJM_LOCATION(10, 31), "SCL X -> X NUMBER");
        njPrintC(NJM_LOCATION(10, 33), "SCL Z -> Z NUMBER");
        toge_count(work);
    } else {
        switch (work->mode) {
        case TOGE_DOWN:
            work->tick++;
            if (work->tick == WAIT_STEPS) {
                work->tick = 0;
                work->mode = TOGE_RISE;
            }
            break;
        case TOGE_RISE:
            VARS(work)->lift += LIFT_STEP;
            work->tick++;
            if (work->tick == MOVE_STEPS) {
                work->tick = 0;
                work->mode = TOGE_UP;
                VARS(work)->lift = FULL_LIFT;
                work->collision->shapes[1].flags &= ~SHAPE_OFF;
            }
            break;
        case TOGE_UP:
            work->tick++;
            if (work->tick == WAIT_STEPS) {
                work->tick = 0;
                work->mode = TOGE_SINK;
                work->collision->shapes[1].flags |= SHAPE_OFF;
            }
            break;
        case TOGE_SINK:
            VARS(work)->lift -= LIFT_STEP;
            work->tick++;
            if (work->tick == MOVE_STEPS) {
                work->tick = 0;
                work->mode = TOGE_DOWN;
                VARS(work)->lift = 0.0F;
            }
            break;
        }
        collision_register(work);
    }
    toge_draw_grid(work);
}

/* count the spikes along x and z from scl.x and scl.z, at least one each */
static void toge_count(struct task_work *work)
{
    Sint16 x = work->scl.x;
    Sint16 z = work->scl.z;

    VARS(work)->count[0] = x < 1 ? 1 : x;
    VARS(work)->count[1] = z < 1 ? 1 : z;
}

/* draw a plate and its spike at each place in the grid, SPACING apart and
 * centered on pos */
static void toge_draw_grid(struct task_work *work)
{
    int i;
    NJS_OBJECT *object = &toge_object;

    draw_attr_save();
    draw_attr_clear(0, NJD_FLAG_IGNORE_SPECULAR);
    texture_set_regular();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateX, FACING(work));
    njTranslate(NULL, (1 - VARS(work)->count[0]) * HALF_SPACING, 0.0F,
                (1 - VARS(work)->count[1]) * HALF_SPACING);
    for (i = 0; i < VARS(work)->count[0]; i++) {
        int j;

        njPushMatrix(NULL);
        for (j = 0; j < VARS(work)->count[1]; j++) {
            njPushMatrix(NULL);
            draw_model(object->model);
            njTranslate(NULL, 0.0F, VARS(work)->lift, 0.0F);
            draw_model(object->child->model);
            njPopMatrix(1);
            njTranslate(NULL, 0.0F, 0.0F, SPACING);
        }
        njPopMatrix(1);
        njTranslate(NULL, SPACING, 0.0F, 0.0F);
    }
    njPopMatrix(1);
    draw_attr_restore();
}

#define BIG_BUBBLE_STEPS 600
/* the most a bubble lives: frames for a small one, which moves only on a
 * frame's first step, and steps for a big one */
#define BUBBLE_LIFE 1200

/* a big bubble grows to its size as it rises this far from O BUBBLE */
#define GROW_HEIGHT 10.0F

/* bubble_find_heights' result bits: a surface above, and ground below */
#define FOUND_SURFACE 0x2
#define FOUND_GROUND 0x4

/* the drops a burst big bubble leaves */
#define BURST_DROPS 4

/* the next ripple starts on a ripple's 15th step */
#define RIPPLE_NEXT 15

/* the sprite is 64 across, and a bubble's size is its radius */
#define SPRITE_SCALE (1.0F / 32.0F)

/* placement_try_leave's distance for a task with no range of its own. a
 * stream leaves once no viewer is within its range plus this */
#define LEAVE_DISTANCE 410.0F

/* a burst's alpha falls from 1 to 0 over FADE_STEPS, and a ripple's from
 * where it starts to -1 */
#define FADE_STEPS 30.0F

#define LERP(lo, hi, t) ((lo) + ((t) * ((hi) - (lo))))

/* the kinds of bubble, small and big, which index bubble_kinds */
enum { BUBBLE_SMALL, BUBBLE_BIG };

/* the ranges a bubble of a kind takes its size, radius and rise from. one
 * random pick sets all three: the higher the pick, the bigger the bubble
 * and its circle, and the slower it rises. a small bubble moves once a
 * frame, and a big one once a step */
struct bubble_kind {
    Float size_min;
    Float size_max;
    /* the most a bubble's turn round its circle changes in a move, either
     * way */
    Angle sway;
    Float radius_min; /* the radius of the circle it rises round */
    Float radius_max;
    Float rise_min; /* how far it rises a move */
    Float rise_max;
};

/* a small bubble, in its stream's free or live list. it rises round its
 * center until it reaches top, the surface above it */
struct bubble {
    NJS_POINT3 pos;
    Float size;
    Float top;
    Sint32 life; /* frames left */
    Float center_x;
    Float center_z;
    Float radius;
    Float rise;
    Angle turn; /* how far round its center it has turned */
    struct bubble *prev;
    struct bubble *next;
};

/* O BUBBLE's view of task_work. search_offset lies over scl.y */
struct bubble_work {
    Sint8 mode;
    Sint8 submode;
    Uint8 id;
    Uint8 tick;
    Sint16 flags;
    /* steps since the last big bubble. it starts at BIG_BUBBLE_STEPS, so the
     * first goes at once */
    Sint16 timer;
    Float surface; /* the height of the surface above it */
    Uint8 unk_0c[8];
    struct angles ang;
    NJS_POINT3 pos;
    Uint8 unk_2c[4];
    Float search_offset; /* how far above it to look for the surface */
    Uint8 unk_34[4];
    struct collision_work *collision;
    struct event_work *event;
};

/* a big bubble's view of task_work. it circles O BUBBLE's x and z */
struct big_bubble_work {
    Sint8 mode;
    Sint8 submode;
    Uint8 id;
    Uint8 tick;
    Sint16 flags;
    Sint16 timer;
    Uint8 unk_08[4];
    Float growth; /* added to its size each step it grows */
    Float radius; /* the radius of the circle it rises round */
    Angle phase; /* the phase of its stretch */
    Angle turn; /* how far round its circle it has turned */
    Sint32 life; /* steps left */
    NJS_POINT3 pos;
    Float size;
    Float stretch; /* how tall it is, for its size */
    Float rise; /* how far it rises a step */
    struct collision_work *collision;
    struct event_work *event;
};

/* O BUBBLES' view of task_work. range, frequency and max lie over scl */
struct bubble_stream_work {
    Sint8 mode;
    Sint8 submode;
    Uint8 id;
    Uint8 tick;
    Sint16 flags;
    /* how many bubbles are live, or -1 once bubble_stream_alloc or
     * bubble_stream_free leaves the stream no pool */
    Sint16 count;
    struct bubble *pool;
    struct bubble *free;
    struct bubble *live;
    struct angles ang;
    NJS_POINT3 pos;
    Float range; /* how far each way across from it a bubble's center lies */
    Float frequency; /* how many start a step */
    /* how many bubbles to make a pool of, or 0 once it's made. the set
     * editor sets it to make the pool again */
    Float max;
    struct collision_work *collision;
    struct event_work *event;
};

/* a drop from a burst big bubble, kept in the burst task's extra */
struct bubble_drop {
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    Float size;
    Bool live; /* TRUE until the drop passes the surface */
};

/* the burst itself, in its task's motion */
struct bubble_burst {
    Float surface; /* the height at which the drops end */
    Float alpha;
};

#define BURST(task) ((struct bubble_burst *)(task)->motion)

/* a ripple on the surface, in its task's extra */
struct bubble_ripple {
    NJS_POINT3 pos;
    Sint32 steps;
    Float fade; /* taken from alpha each step */
    Float alpha;
    Float growth; /* added to scale each step */
    Float start; /* the scale it started at */
    Float scale;
};

static void bubble_step(struct task *task);
static void bubble_destroy(struct task *task);
static void big_bubble_init(struct task *task);
static void big_bubble_grow(struct task *task);
static void big_bubble_rise(struct task *task);
static void big_bubble_draw(struct task *task);
static void big_bubble_set_size(struct big_bubble_work *work, Float size);
static void big_bubble_burst(struct task *task, int player);
static void bubble_stream_step(struct task *task);
static void bubble_stream_draw(struct task *task);
static void bubble_stream_destroy(struct task *task);
static Bool bubble_age_out(struct bubble *bubble);
static void bubble_stream_alloc(struct bubble_stream_work *work, int count);
static void bubble_stream_start(struct bubble_stream_work *work, Float x,
                                Float z);
static struct bubble *bubble_stream_release(struct bubble_stream_work *work,
                                            struct bubble *bubble);
static void bubble_stream_free(struct bubble_stream_work *work);
static void bubble_sway(int kind, NJS_POINT3 *pos, Angle *turn, Float radius,
                        Float center_x, Float center_z, Float rise);
static void bubble_draw_sprite(NJS_POINT3 *pos, NJS_ARGB *color, Float width,
                               Float height);
static int bubble_find_heights(Float *top, Float *bottom, Float x, Float y,
                               Float z);
static Bool bubble_in_reach(struct task *owner, NJS_OBJECT *object, Float x,
                            Float y, Float z);
static void bubble_burst_create(NJS_POINT3 *pos, Float size, Float surface);
static void bubble_burst_random_way(NJS_VECTOR *way);
static void bubble_burst_step(struct task *task);
static void bubble_burst_draw(struct task *task);
static void bubble_ripple_create(NJS_POINT3 *pos, Float growth, Float alpha,
                                 Float scale);
static void bubble_ripple_step(struct task *task);
static void bubble_ripple_draw(struct task *task);
static void bubble_ripple_destroy(struct task *task);

static const struct bubble_kind bubble_kinds[2] = {
    {0.2F, 0.6F, NJM_DEG_ANG(8), 1.0F, 2.0F, 0.4F, 0.8F},
    {5.0F, 5.0F, NJM_DEG_ANG(1), 2.0F, 4.0F, 0.08F, 0.1F},
};

/* the bubbles' material, unless a burst brings its own, and a blank one to
 * clear it after each draw */
static NJS_ARGB bubble_material = {1.0F, 1.0F, 1.0F, 1.0F};
static NJS_ARGB bubble_no_material = {0.0F, 0.0F, 0.0F, 0.0F};
/* one sprite of EFF_REGULAR's bubble texture, placed and scaled for each
 * bubble as it's drawn */
static NJS_TEXLIST bubble_sprite_texlist = {
#include "object/toge_bubble/bubble_sprite_texlist.inc"
};
static NJS_TEXANIM bubble_sprite_texanim[1] = {
#include "object/toge_bubble/bubble_sprite_texanim.inc"
};
static NJS_SPRITE bubble_sprite = {
#include "object/toge_bubble/bubble_sprite.inc"
};

/* the shape a player touches to take a breath from a big bubble. it grows
 * with the bubble */
/* clang-format off */
static struct collision_shape big_bubble_shape = {
    SHAPE_SPHERE | SHAPE_UNTURNED, 0, 0, {0.0F, 0.0F, 0.0F}, 1.0F, 1.0F, 1.0F,
    {0}, 0, NULL};
/* clang-format on */

/* the ripples' material, which bubble_ripple_draw passes to
 * njSetConstantMaterial as one NJS_ARGB from bubble_ripple_alpha on. nothing
 * writes bubble_ripple_alpha, so the material's alpha stays 0.
 * bubble_ripple_draw sets red, green and blue to the ripple's alpha */
/* @hack type: four Floats, not an NJS_ARGB, so that SHC pools each channel's
 * own address for its store (FMOV.S FR4 at 0x8c06d08c, 0x8c06d094 and
 * 0x8c06d098); as one NJS_ARGB, count unmeasured */
static Float bubble_ripple_alpha = 0.0F;
static Float bubble_ripple_red = 0.0F;
static Float bubble_ripple_green = 0.0F;
static Float bubble_ripple_blue = 0.0F;

void bubble_init(struct task *task)
{
    struct bubble_work *work = (struct bubble_work *)task->work;

    /* go no further unless bubble_work keeps these fields where task_work
     * does, as bubble_stream_init and big_bubble_init ask of their views */
    /* @hack redundant: pos tested twice, as the image tests it (ADD #32,R2 at
     * 0x8c06bf0e and 0x8c06bf42); tested once, 40 of 70 words differ */
    /* NOLINTBEGIN(misc-redundant-expression) */
    if (!WORK_SAME_PLACE(struct bubble_work, pos) ||
        !WORK_SAME_PLACE(struct bubble_work, id) ||
        !WORK_SAME_PLACE(struct bubble_work, collision) ||
        !WORK_SAME_PLACE(struct bubble_work, event) ||
        !WORK_SAME_PLACE(struct bubble_work, pos) ||
        !WORK_SAME_PLACE(struct bubble_work, flags)) {
        return;
    }
    /* NOLINTEND(misc-redundant-expression) */
    if (bubble_find_heights(&work->surface, NULL, work->pos.x,
                            work->pos.y + work->search_offset, work->pos.z) &
        FOUND_SURFACE) {
        work->timer = BIG_BUBBLE_STEPS;
        task->step = bubble_step;
        task->draw = NULL;
        task->destructor = bubble_destroy;
    }
}

void bubble_stream_init(struct task *task)
{
    struct bubble_stream_work *work = (struct bubble_stream_work *)task->work;

    /* @hack redundant: pos tested twice, as the image tests it (ADD #32,R2 at
     * 0x8c06bfce and 0x8c06c002); tested once, 37 of 67 words differ */
    /* NOLINTBEGIN(misc-redundant-expression) */
    if (!WORK_SAME_PLACE(struct bubble_stream_work, pos) ||
        !WORK_SAME_PLACE(struct bubble_stream_work, id) ||
        !WORK_SAME_PLACE(struct bubble_stream_work, collision) ||
        !WORK_SAME_PLACE(struct bubble_stream_work, event) ||
        !WORK_SAME_PLACE(struct bubble_stream_work, pos) ||
        !WORK_SAME_PLACE(struct bubble_stream_work, flags)) {
        return;
    }
    /* NOLINTEND(misc-redundant-expression) */
    if (work->max > 0.99F) {
        bubble_stream_alloc(work, (int)work->max);
        work->max = 0.0F;
    }
    task->step = bubble_stream_step;
    task->draw = NULL;
    task->destructor = bubble_stream_destroy;
}

/* let a big bubble go each time the timer passes BIG_BUBBLE_STEPS. while the
 * set editor holds O BUBBLE, find the surface again each step, and show
 * where the search starts and the height it finds */
static void bubble_step(struct task *task)
{
    struct bubble_work *work = (struct bubble_work *)task->work;
    NJS_POINT3 search;

    if (placement_try_leave(task)) {
        return;
    }
    if (++work->timer > BIG_BUBBLE_STEPS) {
        task_create_child(TASK_WORK, big_bubble_init, task);
        work->timer = 0;
    }
    if (placement_editing(task)) {
        search = work->pos;
        search.y += work->search_offset;
        bubble_find_heights(&work->surface, NULL, work->pos.x,
                            work->pos.y + work->search_offset, work->pos.z);
        njPrintC(NJM_LOCATION(17, 20), "<-COLLI SEARCH OFFSET");
        njPrint(NJM_LOCATION(4, 23), "COLLISION HEIGHT %f", work->surface);
        marker_draw(&search, 1.0F);
    }
    task_step_children(task);
}

static void bubble_destroy(struct task *task)
{
}

/* pick a big bubble's size, circle and rise from its kind. it starts at
 * size 0 */
static void big_bubble_init(struct task *task)
{
    struct big_bubble_work *work = (struct big_bubble_work *)task->work;
    const struct bubble_kind *kind = &bubble_kinds[BUBBLE_BIG];
    Float pick;
    Float size;
    Float rise;

    /* @hack redundant: pos tested twice, as the image tests it (ADD #32,R2 at
     * 0x8c06c192 and 0x8c06c1c4); tested once, 83 of 113 words differ */
    /* NOLINTBEGIN(misc-redundant-expression) */
    if (!WORK_SAME_PLACE(struct big_bubble_work, pos) ||
        !WORK_SAME_PLACE(struct big_bubble_work, id) ||
        !WORK_SAME_PLACE(struct big_bubble_work, collision) ||
        !WORK_SAME_PLACE(struct big_bubble_work, event) ||
        !WORK_SAME_PLACE(struct big_bubble_work, pos) ||
        !WORK_SAME_PLACE(struct big_bubble_work, flags)) {
        return;
    }
    /* NOLINTEND(misc-redundant-expression) */
    pick = rand() * RAND_SCALE;
    size = LERP(kind->size_min, kind->size_max, pick);
    work->radius = LERP(kind->radius_min, kind->radius_max, pick);
    rise = LERP(kind->rise_min, kind->rise_max, 1.0F - (pick * pick));
    work->size = 0.0F;
    work->growth = size * rise * (1.0F / GROW_HEIGHT);
    work->rise = rise;
    work->life = BUBBLE_LIFE;
    collision_init(task, &big_bubble_shape, COLLISION_OBJECT, 1, 1);
    task->step = big_bubble_grow;
    task->draw = big_bubble_draw;
    task->destructor = bubble_destroy;
}

/* grow while rising, until the bubble is GROW_HEIGHT above O BUBBLE */
static void big_bubble_grow(struct task *task)
{
    struct bubble_work *parent = (struct bubble_work *)task->parent->work;
    struct big_bubble_work *work = (struct big_bubble_work *)task->work;

    work->size += work->growth;
    big_bubble_set_size(work, work->size);
    big_bubble_rise(task);
    /* @bug memory: big_bubble_rise can destroy the task (BSR at
     * 0x8c06c2e6), and the test still reads its freed work (0x8c06c2f4)
     * and can store big_bubble_rise as the freed task's step (0x8c06c2fe) */
    if (work->pos.y > parent->pos.y + GROW_HEIGHT) {
        task->step = big_bubble_rise;
    }
}

/* rise round its circle, stretching as it goes. a player who touches it
 * bursts it, and at the surface it leaves a ripple. it ends when its life
 * runs out */
static void big_bubble_rise(struct task *task)
{
    struct bubble_work *parent;
    struct big_bubble_work *work;
    Float surface;
    Float sine;
    Float cosine;

    parent = (struct bubble_work *)task->parent->work;
    work = (struct big_bubble_work *)task->work;
    surface = parent->surface;
    work->phase += NJM_DEG_ANG(5);
    _builtin_fsca(work->phase, &sine, &cosine);
    work->stretch = LERP(0.85F, 1.0F, 0.5F + (sine * 0.5F));
    bubble_sway(BUBBLE_BIG, &work->pos, &work->turn, work->radius,
                parent->pos.x, parent->pos.z, work->rise);
    /* @hack spelling: the two tests nested, which has SHC read
     * work->collision again for the second (MOV.L @(56,R14),R2 at
     * 0x8c06c37e); joined with &&, SHC keeps it in R4 from the first, the
     * function runs 2 bytes short and 17 of 119 words differ */
    if (work->collision->flags & COLLISION_HIT) {
        if (work->collision->hit->kind == COLLISION_PLAYER) {
            big_bubble_burst(
                task, PLAYER_VARS(work->collision->hit->owner->work)->player);
            task_destroy(task);
            return;
        }
    }
    if (surface <= work->pos.y + (work->size * 0.6F)) {
        work->pos.y = surface;
        bubble_ripple_create(&work->pos, 0.05F, 0.5F, work->size * 0.2F);
        task_destroy(task);
        return;
    }
    if (--work->life < 0) {
        task_destroy(task);
        return;
    }
    collision_register((struct task_work *)work);
    big_bubble_draw(task);
}

static void big_bubble_draw(struct task *task)
{
    struct big_bubble_work *work = (struct big_bubble_work *)task->work;

    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    bubble_draw_sprite(&work->pos, NULL, work->size,
                       work->stretch * work->size);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
}

/* size the shape to the bubble, and the collision record's radius to 8
 * more */
static void big_bubble_set_size(struct big_bubble_work *work, Float size)
{
    struct collision_shape *shape = work->collision->shapes;

    shape->radius = size;
    work->collision->radius = size + 8.0F;
}

/* the player takes a breath, and the bubble bursts into drops */
static void big_bubble_burst(struct task *task, int player)
{
    struct bubble_work *parent = (struct bubble_work *)task->parent->work;
    struct big_bubble_work *work = (struct big_bubble_work *)task->work;

    player_set_mode(player, PLAYER_MODE_BREATHE);
    bubble_burst_create(&work->pos, work->size, parent->surface);
}

/* start bubbles as often as frequency says, and on a frame's first step
 * move and draw the live ones. while the set editor holds the stream, make
 * the pool again when max asks, and show the settings */
static void bubble_stream_step(struct task *task)
{
    struct bubble_stream_work *work = (struct bubble_stream_work *)task->work;
    Float chance;
    struct bubble *bubble;
    int i;

    if (placement_try_leave_by(task, SQUARE(work->range + LEAVE_DISTANCE))) {
        return;
    }
    /* a bubble for each whole unit of the frequency, and one more with the
     * odds its fraction gives */
    chance = work->frequency;
    while (chance > 0.0F) {
        if (chance > rand() * RAND_SCALE) {
            bubble_stream_start(work, work->pos.x, work->pos.z);
        }
        chance -= 1.0F;
    }
    if (!frame_step) {
        bubble = work->live;
        i = work->count;
        /* @hack spelling: the loop as a do inside an if, which counts down with
         * DT R11 before it tests the next bubble (0x8c06c584 to 0x8c06c58a);
         * as a for loop, the function runs 2 bytes long and 59 of 138 words
         * differ */
        if (i > 0) {
            do {
                if (bubble_age_out(bubble)) {
                    bubble = bubble_stream_release(work, bubble);
                } else {
                    bubble_sway(BUBBLE_SMALL, &bubble->pos, &bubble->turn,
                                bubble->radius, bubble->center_x,
                                bubble->center_z, bubble->rise);
                    bubble = bubble->next;
                }
            } while (--i != 0 && bubble != NULL);
        }
        bubble_stream_draw(task);
    }
    if (placement_editing(task)) {
        if (work->max > 0.99F) {
            bubble_stream_free(work);
            bubble_stream_alloc(work, (int)work->max);
            work->max = 0.0F;
        }
        njPrintC(NJM_LOCATION(17, 19), "<-RANGE");
        njPrintC(NJM_LOCATION(17, 20), "<-FREQUENCY");
        njPrint(NJM_LOCATION(17, 21), "<-MAX (DISP %d)", work->count);
    }
}

static void bubble_stream_draw(struct task *task)
{
    struct bubble_stream_work *work = (struct bubble_stream_work *)task->work;
    struct bubble *bubble;
    Float scale;
    int i;

    /* @hack redundant: the else after the return, which loads
     * njSetConstantMaterial's address into R2 (0x8c06c636); without it, R3
     * takes it and 2 of 82 words differ */
    if (frame_step ||
        placement_try_leave_by(task, SQUARE(work->range + LEAVE_DISTANCE))) {
        return;
    } else {
        bubble = work->live;
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njSetConstantMaterial(&bubble_material);
        i = work->count;
        /* @hack spelling: the loop as a do inside an if, which tests i before
         * it loads bubble_sprite's and njDrawSprite3D's addresses (CMP/PL
         * R12 at 0x8c06c642), and counts down with DT (0x8c06c66e); as a for
         * loop, the function runs 2 bytes long and 30 of 82 words differ */
        if (i > 0) {
            do {
                scale = bubble->size * SPRITE_SCALE;
                bubble_sprite.p = bubble->pos;
                bubble_sprite.sx = scale;
                bubble_sprite.sy = scale;
                njDrawSprite3D(&bubble_sprite, 0,
                               NJD_SPRITE_COLOR | NJD_SPRITE_SCALE |
                                   NJD_SPRITE_ALPHA);
                bubble = bubble->next;
            } while (--i != 0);
        }
        njSetConstantMaterial(&bubble_no_material);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
    }
}

static void bubble_stream_destroy(struct task *task)
{
    bubble_stream_free((struct bubble_stream_work *)task->work);
}

/* spend a frame of a small bubble's life while it's below the surface. it
 * returns TRUE once the bubble reaches the surface or runs out of life, or
 * comes within size + 2 of player 0 or 1. it returns FALSE otherwise */
static Bool bubble_age_out(struct bubble *bubble)
{
    NJS_POINT3 player;
    /* @hack spelling: bubble's pos through a pointer of its own. SHC copies R4
     * to R13 for bubble and to R12 for pos (0x8c06c71c, 0x8c06c722), and
     * saves R9 as well (0x8c06c712); through bubble alone, the function
     * runs 6 bytes short and 54 of 59 words differ */
    NJS_POINT3 *pos = &bubble->pos;
    int i;

    if (pos->y >= bubble->top || --bubble->life < 0) {
        return TRUE;
    }
    for (i = 0; i <= 1; i++) {
        player_get_history(i, 0, &player, NULL);
        if (point_distance_squared(&player, pos) <
            SQUARE(bubble->size + 2.0F)) {
            return TRUE;
        }
    }
    return FALSE;
}

/* make a pool of count bubbles, all free. with count under 1, or no room in
 * the heap, the stream has no pool and its count is -1 */
static void bubble_stream_alloc(struct bubble_stream_work *work, int count)
{
    struct bubble *pool;
    struct bubble *bubble;
    int i;

    work->live = NULL;
    if (count > 0 && (pool = heap_calloc(count, sizeof(*pool))) != NULL) {
        bubble = pool;
        bubble->prev = NULL;
        bubble->next = bubble + 1;
        bubble++;
        for (i = 1; i < count; i++) {
            bubble->prev = bubble - 1;
            bubble->next = bubble + 1;
            bubble++;
        }
        pool[count - 1].next = NULL;
        work->count = 0;
        work->free = pool;
        work->pool = pool;
    } else {
        work->count = -1;
        work->free = NULL;
        work->pool = NULL;
    }
}

/* start a free bubble on the ground below x, z, circling a point within
 * range of it each way across. it starts only where it finds ground below
 * and a surface above */
static void bubble_stream_start(struct bubble_stream_work *work, Float x,
                                Float z)
{
    struct bubble *bubble = work->free;
    const struct bubble_kind *kind;
    Float pick;

    if (bubble == NULL) {
        return;
    }
    if ((bubble_find_heights(&bubble->top, &bubble->pos.y, x, work->pos.y, z) &
         (FOUND_SURFACE | FOUND_GROUND)) != (FOUND_SURFACE | FOUND_GROUND)) {
        return;
    }
    work->count++;
    work->free = bubble->next;
    bubble->next = work->live;
    bubble->prev = NULL;
    if (work->live != NULL) {
        work->live->prev = bubble;
    }
    work->live = bubble;
    bubble->center_x =
        (rand() * work->range * (2.0F * RAND_SCALE)) - work->range + x;
    bubble->center_z =
        (rand() * work->range * (2.0F * RAND_SCALE)) - work->range + z;
    kind = &bubble_kinds[BUBBLE_SMALL];
    pick = rand() * RAND_SCALE;
    bubble->size = LERP(kind->size_min, kind->size_max, pick);
    bubble->radius = LERP(kind->radius_min, kind->radius_max, pick);
    bubble->rise = LERP(kind->rise_min, kind->rise_max, 1.0F - (pick * pick));
    bubble->life = BUBBLE_LIFE;
}

/* move bubble from the live list to the free list. it returns the live
 * bubble after it, or NULL */
static struct bubble *bubble_stream_release(struct bubble_stream_work *work,
                                            struct bubble *bubble)
{
    /* @hack spelling: head and prev read up front, ahead of the first test
     * (0x8c06c942, 0x8c06c944); read where they're used, each loads again
     * at each use, the function runs 8 bytes long and 23 of 25 words
     * differ */
    struct bubble *head = work->free;
    struct bubble *prev = bubble->prev;
    struct bubble *next = bubble->next;

    if (work->live == bubble) {
        work->live = next;
    }
    if (prev != NULL) {
        prev->next = next;
    }
    if (next != NULL) {
        next->prev = prev;
    }
    bubble->prev = NULL;
    bubble->next = head;
    if (head != NULL) {
        head->prev = bubble;
    }
    work->free = bubble;
    work->count--;
    return next;
}

static void bubble_stream_free(struct bubble_stream_work *work)
{
    heap_free(work->pool);
    work->count = -1;
    work->free = NULL;
    work->live = NULL;
    work->pool = NULL;
}

/* move a bubble on round its circle about center_x, center_z, and up by
 * rise. its turn changes by up to its kind's sway either way */
static void bubble_sway(int kind, NJS_POINT3 *pos, Angle *turn, Float radius,
                        Float center_x, Float center_z, Float rise)
{
    Float sine;
    Float cosine;
    Angle sway;

    /* @hack spelling: the sway in a local of its own, which keeps turn's
     * address on the stack and reads it there (MOV.L @(8,R15),R3 at
     * 0x8c06c9c4); added in place as an Angle, SHC keeps the address in R8,
     * which it saves, the function runs 4 bytes long and 66 of 75 words
     * differ */
    sway = ((Float)rand() * bubble_kinds[kind].sway * (2.0F * RAND_SCALE)) -
           bubble_kinds[kind].sway;
    *turn += sway;
    _builtin_fsca(*turn, &sine, &cosine);
    pos->x = center_x + (radius * cosine);
    pos->z = center_z + (radius * sine);
    pos->y += rise;
}

/* draw a bubble at pos, width by height, in color, or in bubble_material
 * for NULL. it draws only on a frame's first step */
static void bubble_draw_sprite(NJS_POINT3 *pos, NJS_ARGB *color, Float width,
                               Float height)
{
    if (frame_step) {
        return;
    }
    if (color == NULL) {
        color = &bubble_material;
    }
    njSetConstantMaterial(color);
    bubble_sprite.p = *pos;
    bubble_sprite.sx = width * SPRITE_SCALE;
    bubble_sprite.sy = height * SPRITE_SCALE;
    njDrawSprite3D(&bubble_sprite, 0,
                   NJD_SPRITE_COLOR | NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
    njSetConstantMaterial(&bubble_no_material);
}

/* find the nearest surface above x, y, z and the nearest ground below it.
 * it looks among the objects world_list_near lists within 20 of the point,
 * or within 50 for a task's object. it puts the heights in top and bottom,
 * either of which may be NULL. a side with nothing found takes GROUND_FAR,
 * or -GROUND_FAR below. it returns FOUND_SURFACE and FOUND_GROUND for the
 * sides it finds */
static int bubble_find_heights(Float *top, Float *bottom, Float x, Float y,
                               Float z)
{
    struct height_query query;
    int found = 0;
    Float above;
    Float below;
    Float high;
    Float low;
    int left;
    struct world_object *entry;
    struct task *owner;
    NJS_OBJECT *object;

    query.pos.x = x;
    query.pos.y = y;
    query.pos.z = z;
    world_list_near(x, y, z, 20.0F);
    high = GROUND_FAR;
    low = -GROUND_FAR;
    left = world_count;
    entry = world_objects;
    /* @bug memory: the walk tests left before it counts down (TST
     * R10,R10 at 0x8c06cb96, ADD #-1,R10 at 0x8c06cb9c), so it takes
     * world_count + 1 entries, one more than world_list_near listed. the
     * last lies past the list: an entry an earlier, longer list left there,
     * or zeros */
    if (left != 0) {
        do {
            object = entry->object;
            owner = entry->owner;
            /* @hack spelling: the two tests nested, which keep FOUND_GROUND
             * in R9 and FOUND_SURFACE in R8 (0x8c06cb34, 0x8c06cb36); joined
             * with &&, R8 and R9 trade them and 4 of 120 words differ */
            if (bubble_in_reach(owner, object, x, y, z)) {
                if (polygon_find_heights(&query, object, entry->flags)) {
                    above = query.above.level;
                    below = query.below.level;
                    /* add the owner's speed.y: a moving object's surfaces
                     * move with it */
                    if (owner != NULL) {
                        above += ((struct task_force *)owner->force)->speed.y;
                        below += ((struct task_force *)owner->force)->speed.y;
                    }
                    if (query.above.found && above > y && above < high) {
                        high = above;
                        found |= FOUND_SURFACE;
                    }
                    if (query.below.found && y > below && below > low) {
                        low = below;
                        found |= FOUND_GROUND;
                    }
                }
            }
            entry++;
        } while (left-- != 0);
    }
    if (top != NULL) {
        *top = high;
    }
    if (bottom != NULL) {
        *bottom = low;
    }
    return found;
}

/* TRUE when x, z lies within the object's reach across, its model's radius
 * times its largest scale, and FALSE otherwise. the land, which has no
 * owner, is always in reach. y goes unread */
static Bool bubble_in_reach(struct task *owner, NJS_OBJECT *object, Float x,
                            Float y, Float z)
{
    Float dx;
    Float dz;
    Float distance;
    Float scale;
    Float reach;

    if (owner == NULL) {
        return TRUE;
    }
    dx = object->pos[0] - x;
    dz = object->pos[2] - z;
    distance = (dx * dx) + (dz * dz);
    scale = object->scl[0] >= object->scl[1] ? object->scl[0] : object->scl[1];
    if (object->scl[2] > scale) {
        scale = object->scl[2];
    }
    reach = object->model->r * object->model->r;
    scale = scale * scale * reach;
    if (scale > distance) {
        return TRUE;
    }
    return FALSE;
}

/* burst a big bubble at pos into BURST_DROPS drops, each of a random size
 * and thrown out a random way up. size goes unread. with no room for the
 * task or its records, nothing bursts */
static void bubble_burst_create(NJS_POINT3 *pos, Float size, Float surface)
{
    struct task *task;
    struct bubble_drop *drop;
    struct bubble_burst *burst;
    NJS_VECTOR way;
    int i;

    task = task_create(0, 6, bubble_burst_step);
    if (task == NULL) {
        return;
    }
    drop = heap_alloc(sizeof(struct bubble_drop) * BURST_DROPS);
    burst = heap_alloc(sizeof(struct bubble_burst));
    if (drop != NULL && burst != NULL) {
        /* @hack spelling: the count set first, which builds its 4 in R11 and
         * copies it to R0 for alpha's offset (MOV #4,R11 at 0x8c06ccae, MOV
         * R11,R0 at 0x8c06ccb2); set at the loop, SHC builds the 4 twice,
         * FR13 and FR14 trade, and 6 of 160 words and 4 of 20 pool words
         * differ */
        i = BURST_DROPS;
        task->extra = drop;
        task->motion = (struct task_motion *)burst;
        burst->alpha = 1.0F;
        burst->surface = surface;
        do {
            drop->live = TRUE;
            drop->size = RANDOM(0.2F, 1.5F);
            bubble_burst_random_way(&way);
            VECTOR_SCALE(&drop->speed, &way, 0.1F);
            VECTOR_SCALE(&way, &way, drop->size);
            VECTOR_SUM(&drop->pos, &way, pos);
            drop++;
        } while (--i != 0);
    } else {
        heap_free(burst);
        heap_free(drop);
        task_destroy(task);
    }
}

/* a unit vector any way round, pointing up */
static void bubble_burst_random_way(NJS_VECTOR *way)
{
    Angle ang;
    Float sin_y;
    Float cos_y;
    Float sin_x;
    Float cos_x;

    ang = RANDOM_ANGLE();
    _builtin_fsca(ang, &sin_y, &cos_y);
    /* tilted up from the horizontal by a random angle under half a turn */
    ang = rand() * (RAND_SCALE * NJM_DEG_ANG(180));
    _builtin_fsca(ang, &sin_x, &cos_x);
    sin_y *= cos_x;
    cos_y *= cos_x;
    way->x = sin_y;
    way->y = sin_x;
    way->z = cos_y;
}

/* move the live drops, each until it passes the surface, while the
 * burst's alpha is above 0. the alpha reaches 0 on the 30th step, so the
 * drops rise and fade for 29 steps at most. the burst ends on the step no
 * live drop is left, or on the 30th */
static void bubble_burst_step(struct task *task)
{
    struct bubble_burst *burst = BURST(task);
    struct bubble_drop *drop = task->extra;
    Bool done = TRUE;
    int i;
    Float surface;

    burst->alpha -= 1.0F / FADE_STEPS;
    if (burst->alpha > 0.0F) {
        surface = burst->surface;
        i = BURST_DROPS;
        do {
            if (drop->live) {
                VECTOR_SUM(&drop->pos, &drop->pos, &drop->speed);
                drop->pos.y += 0.15F;
                if (drop->pos.y > surface) {
                    drop->live = FALSE;
                } else {
                    done = FALSE;
                }
            }
            drop++;
        } while (--i != 0);
    }
    if (done) {
        task_destroy(task);
    } else {
        bubble_burst_draw(task);
    }
}

static void bubble_burst_draw(struct task *task)
{
    struct bubble_drop *drop = task->extra;
    struct bubble_burst *burst = BURST(task);
    NJS_ARGB color;
    int i;
    Float alpha;
    Float size;

    /* @hack spelling: the count set first, which builds its 4 in R13 and copies
     * it to R0 to read alpha at offset 4 (MOV #4,R13 at 0x8c06ceca, MOV
     * R13,R0 at 0x8c06cece); set at the loop, SHC builds that 4 in R0 and
     * 17 of 52 words differ */
    i = BURST_DROPS;
    alpha = burst->alpha;
    color.a = 1.0F;
    color.r = alpha;
    color.g = alpha;
    color.b = alpha;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    for (; i != 0; i--) {
        if (drop->live) {
            size = drop->size;
            bubble_draw_sprite(&drop->pos, &color, size, size);
        }
        drop++;
    }
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
}

/* a ripple at pos, from scale and alpha, which grows by growth a step and
 * fades as it spreads */
static void bubble_ripple_create(NJS_POINT3 *pos, Float growth, Float alpha,
                                 Float scale)
{
    struct task *task = task_create(0, 6, bubble_ripple_step);
    struct bubble_ripple *ripple;

    task->draw = bubble_ripple_draw;
    task->destructor = bubble_ripple_destroy;
    task->extra = heap_alloc(sizeof(struct bubble_ripple));
    if (task->extra != NULL) {
        ripple = task->extra;
        ripple->pos = *pos;
        ripple->growth = growth;
        ripple->start = scale;
        ripple->scale = scale;
        ripple->alpha = alpha;
        ripple->steps = 0;
        ripple->fade = (alpha + 1.0F) * (1.0F / FADE_STEPS);
    } else {
        task_destroy(task);
    }
}

/* spread and fade. at RIPPLE_NEXT, start the next ripple from the scale
 * this one started at and its alpha now. end once alpha falls below -0.9 */
static void bubble_ripple_step(struct task *task)
{
    struct bubble_ripple *ripple = task->extra;
    Float alpha;
    Float scale;

    scale = ripple->scale + ripple->growth;
    ripple->scale = scale;
    /* @hack spelling: alpha read in a statement of its own, ahead of fade (MOV
     * #20,R0 and FMOV.S @(R0,R14),FR15 at 0x8c06d004); as one expression,
     * SHC reads fade first and 4 of 48 words differ */
    alpha = ripple->alpha;
    alpha -= ripple->fade;
    if (alpha < -0.9F) {
        task_destroy(task);
        return;
    }
    if (++ripple->steps == RIPPLE_NEXT) {
        bubble_ripple_create(&ripple->pos, ripple->growth, alpha,
                             ripple->start);
    }
    ripple->alpha = alpha;
    bubble_ripple_draw(task);
}

/* draw the ripple's model at its scale, its alpha as red, green and blue,
 * added onto what lies behind it. it draws only on a frame's first step */
static void bubble_ripple_draw(struct task *task)
{
    struct bubble_ripple *ripple = task->extra;
    Float alpha;
    Float scale;

    if (frame_step) {
        return;
    }
    texture_set_regular();
    njPushMatrix(NULL);
    njTranslateV(NULL, &ripple->pos);
    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
    alpha = ripple->alpha;
    bubble_ripple_red = alpha;
    bubble_ripple_green = alpha;
    bubble_ripple_blue = alpha;
    njSetConstantMaterial((NJS_ARGB *)&bubble_ripple_alpha);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    scale = ripple->scale;
    njScale(NULL, scale, 1.0F, scale);
    njDrawObject(&ripple_object);
    njSetConstantMaterial(&bubble_no_material);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    draw_attr_restore();
    njPopMatrix(1);
}

static void bubble_ripple_destroy(struct task *task)
{
}

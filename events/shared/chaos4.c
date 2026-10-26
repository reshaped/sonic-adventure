/* Chaos 4 as the events pose him, with welds that close his model's seams
 * in each pose */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/ground.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "player/player.h"
#include "shared/chaos0_water_textures.h"
#include "shared/chaos4.h"
#include "shared/weld.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 10
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 10
#define TEXT_7_LENGTH 10
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 10
#include "shared/chaos4/text.inc"

/* the most nodes event_weld poses. his model has 79, so it poses them all */
#define CHAOS4_NODES 92
/* the share of the puddle's x and z scale its wobble sways it by */
#define CHAOS4_SWAY 0.125F

/* what his step does, by work->mode */
enum chaos4_mode {
    CHAOS4_STAND, /* he loops his action */
    CHAOS4_MELT, /* his body sinks as the puddle grows */
    CHAOS4_RISE, /* his body rises as the puddle shrinks */
    CHAOS4_PUDDLE, /* the puddle alone, wobbling in place */
    CHAOS4_SLIDE /* the puddle alone, wobbling as speed moves it */
};

/* his state, which event_chaos4_create allocates and his destructor frees */
struct chaos4_state {
    NJS_POINT3 to; /* where the slide is headed. nothing reads it */
    NJS_VECTOR speed; /* how far the slide moves him each step */
    Sint32 steps; /* the slide lasts steps + 1 steps */
    Sint32 form_steps; /* the steps a melt or a rise divides its change into */
    Float action_speed; /* how many frames of his action play each step */
    /* added to half of event_chaos4_depth, how far above the puddle his body
     * draws */
    Float lift;
    Float lift_speed;
    NJS_VECTOR puddle_scale;
    NJS_VECTOR form_speed;
    NJS_VECTOR form; /* the puddle's scale as he melts and rises */
    /* the puddle's alpha as he melts and rises. no draw reads it */
    Float alpha;
    Float alpha_speed;
    Angle wobble; /* the phase of the wobble that sways the puddle's scale */
};

struct chaos4_vars {
    Float frame; /* his action's frame */
    Sint32 timer; /* steps into a melt, a rise or the slide */
    struct chaos4_state *state;
};

#define VARS(work) ((struct chaos4_vars *)&(work)->vars)

/* his model and the nodes the welds join, which each event defines in its
 * own data. the model has 79 nodes. a node's name gives its place in the
 * model's walk, as tree_node_index counts it, from 0 at event_chaos4_object */
extern NJS_OBJECT event_chaos4_object;
extern NJS_OBJECT event_chaos4_node3;
extern NJS_OBJECT event_chaos4_node8;
extern NJS_OBJECT event_chaos4_node12;
extern NJS_OBJECT event_chaos4_node22;
extern NJS_OBJECT event_chaos4_node24;
extern NJS_OBJECT event_chaos4_node26;
extern NJS_OBJECT event_chaos4_node32;
extern NJS_OBJECT event_chaos4_node44;
extern NJS_OBJECT event_chaos4_node46;
extern NJS_OBJECT event_chaos4_node47;
extern NJS_OBJECT event_chaos4_node48;
extern NJS_OBJECT event_chaos4_node60;
extern NJS_OBJECT event_chaos4_node62;
extern NJS_OBJECT event_chaos4_node63;
extern NJS_OBJECT event_chaos4_node64;
/* @hack type: Chaos 0's puddle, event_chaos0_puddle_object, declared as an
 * array, which the draw passes by its name. each event defines it. SHC then
 * loads its address once, into R8, for the draw's four draw_tree calls
 * (0x0cb83370 in EV000B). as one NJS_OBJECT passed as
 * &event_chaos0_puddle_object, the code runs 8 bytes short, and 220 of the
 * draw's 322 words differ */
extern NJS_OBJECT event_chaos0_puddle_object[];

/* how far a melt sinks his body in the steps create took */
static const Float event_chaos4_depth = 55.0F;

extern NJS_MDATA3 event_chaos4_mdata[];

static NJS_MOTION event_chaos4_motion = {
#include "shared/chaos4/event_chaos4_motion.inc"
};
static NJS_ACTION event_chaos4_action = {
#include "shared/chaos4/event_chaos4_action.inc"
};
static NJS_TEXNAME event_chaos4_textures[9] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0}, {TEXT_6, 0, 0},
    {TEXT_7, 0, 0}, {TEXT_8, 0, 0}, {TEXT_9, 0, 0},
};
NJS_TEXLIST event_chaos4_texlist = {
#include "shared/chaos4/event_chaos4_texlist.inc"
};

/* the last Chaos 4 created. nothing clears it */
static struct task *event_chaos4_task = NULL;

/* each weld's vertex pairs: a vertex's index in object_a's model, then its
 * index in object_b's. a name gives the two nodes, object_a's first */
static Uint16 event_chaos4_indices_8_12[16] = {
#include "shared/chaos4/event_chaos4_indices_8_12.inc"
};
static Uint16 event_chaos4_indices_12_3[16] = {
#include "shared/chaos4/event_chaos4_indices_12_3.inc"
};
static Uint16 event_chaos4_indices_3_32[12] = {
#include "shared/chaos4/event_chaos4_indices_3_32.inc"
};
static Uint16 event_chaos4_indices_32_26[12] = {
#include "shared/chaos4/event_chaos4_indices_32_26.inc"
};
static Uint16 event_chaos4_indices_26_24[12] = {
#include "shared/chaos4/event_chaos4_indices_26_24.inc"
};
static Uint16 event_chaos4_indices_22_24[12] = {
#include "shared/chaos4/event_chaos4_indices_22_24.inc"
};
static Uint16 event_chaos4_indices_3_64[10] = {
#include "shared/chaos4/event_chaos4_indices_3_64.inc"
};
static Uint16 event_chaos4_indices_64_63[10] = {
#include "shared/chaos4/event_chaos4_indices_64_63.inc"
};
static Uint16 event_chaos4_indices_63_62[10] = {
#include "shared/chaos4/event_chaos4_indices_63_62.inc"
};
static Uint16 event_chaos4_indices_62_60[10] = {
#include "shared/chaos4/event_chaos4_indices_62_60.inc"
};
static Uint16 event_chaos4_indices_3_48[10] = {
#include "shared/chaos4/event_chaos4_indices_3_48.inc"
};

/* the welds of nodes 48 to 44 reuse the pairs of the welds of nodes 64 to 60.
 * the last three welds' indices are named for those other nodes */
static struct weld event_chaos4_welds[15] = {
    {&event_chaos4_object, &event_chaos4_node8, &event_chaos4_node12, 8,
     WELD_BOTH, 0, 0, NULL, event_chaos4_indices_8_12},
    {&event_chaos4_object, &event_chaos4_node12, &event_chaos4_node3, 8,
     WELD_BOTH, 0, 0, NULL, event_chaos4_indices_12_3},
    {&event_chaos4_object, &event_chaos4_node3, &event_chaos4_node32, 6,
     WELD_BOTH, 0, 0, NULL, event_chaos4_indices_3_32},
    {&event_chaos4_object, &event_chaos4_node32, &event_chaos4_node26, 6,
     WELD_BOTH, 0, 0, NULL, event_chaos4_indices_32_26},
    {&event_chaos4_object, &event_chaos4_node26, &event_chaos4_node24, 6,
     WELD_BOTH, 0, 0, NULL, event_chaos4_indices_26_24},
    {&event_chaos4_object, &event_chaos4_node22, &event_chaos4_node24, 6,
     WELD_BOTH, 0, 0, NULL, event_chaos4_indices_22_24},
    {&event_chaos4_object, &event_chaos4_node3, &event_chaos4_node64, 5,
     WELD_BOTH, 0, 0, NULL, event_chaos4_indices_3_64},
    {&event_chaos4_object, &event_chaos4_node64, &event_chaos4_node63, 5,
     WELD_BOTH, 0, 0, NULL, event_chaos4_indices_64_63},
    {&event_chaos4_object, &event_chaos4_node63, &event_chaos4_node62, 5,
     WELD_BOTH, 0, 0, NULL, event_chaos4_indices_63_62},
    {&event_chaos4_object, &event_chaos4_node62, &event_chaos4_node60, 5,
     WELD_BOTH, 0, 0, NULL, event_chaos4_indices_62_60},
    {&event_chaos4_object, &event_chaos4_node3, &event_chaos4_node48, 5,
     WELD_BOTH, 0, 0, NULL, event_chaos4_indices_3_48},
    {&event_chaos4_object, &event_chaos4_node48, &event_chaos4_node47, 5,
     WELD_BOTH, 0, 0, NULL, event_chaos4_indices_64_63},
    {&event_chaos4_object, &event_chaos4_node47, &event_chaos4_node46, 5,
     WELD_BOTH, 0, 0, NULL, event_chaos4_indices_63_62},
    {&event_chaos4_object, &event_chaos4_node46, &event_chaos4_node44, 5,
     WELD_BOTH, 0, 0, NULL, event_chaos4_indices_62_60},
    {NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL},
};

static void event_chaos4_init(struct task *task);
static void event_chaos4_step(struct task *task);
static void event_chaos4_draw(struct task *task);
static void event_chaos4_destroy(struct task *task);

void event_chaos4_load_textures(void)
{
    njLoadTexture(&event_chaos4_texlist);
    njLoadTexture(&event_chaos0_water_texlist);
}

void event_chaos4_release_textures(void)
{
    njReleaseTexture(&event_chaos4_texlist);
    njReleaseTexture(&event_chaos0_water_texlist);
}

void event_chaos4_create(Float x, Float y, Float z, Angle ang_x, Angle ang_y,
                         Angle ang_z, int steps)
{
    struct chaos4_state *state;
    struct task_work *work;

    event_chaos4_task = task_create(TASK_WORK, 5, event_chaos4_init);
    state = heap_alloc(sizeof(struct chaos4_state));
    work = event_chaos4_task->work;
    VARS(work)->state = state;
    work->mode = CHAOS4_STAND;
    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    work->ang.x = ang_x;
    work->ang.y = ang_y;
    work->ang.z = ang_z;
    VARS(work)->frame = 0.0F;
    state->action_speed = 0.1F;
    state->form_steps = steps;
    state->lift = 0.0F;
    state->lift_speed = event_chaos4_depth / state->form_steps;
    state->puddle_scale.x = 7.0F;
    state->puddle_scale.y = 1.0F;
    state->puddle_scale.z = 7.0F;
    state->form_speed.x = 0.4F / state->form_steps;
    state->form_speed.y = 1.0F / state->form_steps;
    state->form_speed.z = 0.4F / state->form_steps;
    state->form.x = 0.8F;
    state->form.y = 0.01F;
    state->form.z = 0.8F;
    state->alpha = 0.05F;
    state->alpha_speed = 0.5F / state->form_steps;
    state->wobble = 0;
    event_weld(event_chaos4_welds, &event_chaos4_action, VARS(work)->frame,
               CHAOS4_NODES, PLAYER_WELD_MAKE);
}

void event_chaos4_kill(void)
{
    if (event_chaos4_task != NULL) {
        task_kill(event_chaos4_task);
    }
}

void event_chaos4_melt(void)
{
    struct task_work *work = event_chaos4_task->work;

    VARS(work)->timer = 0;
    work->mode = CHAOS4_MELT;
}

void event_chaos4_rise(void)
{
    struct task_work *work = event_chaos4_task->work;

    VARS(work)->timer = 0;
    work->mode = CHAOS4_RISE;
}

void event_chaos4_set_speed(Float speed)
{
    struct task_work *work = event_chaos4_task->work;
    struct chaos4_state *state = VARS(work)->state;

    state->action_speed = speed;
}

void event_chaos4_set_pos(Float x, Float y, Float z)
{
    struct task_work *work = event_chaos4_task->work;

    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
}

void event_chaos4_set_ang(Angle x, Angle y, Angle z)
{
    struct task_work *work = event_chaos4_task->work;

    work->ang.x = x;
    work->ang.y = y;
    work->ang.z = z;
}

void event_chaos4_slide(Float x, Float y, Float z, int steps)
{
    struct task_work *work = event_chaos4_task->work;
    struct chaos4_state *state = VARS(work)->state;

    state->to.x = x;
    state->to.y = y;
    state->to.z = z;
    state->speed.x = (x - work->pos.x) / steps;
    state->speed.y = (y - work->pos.y) / steps;
    state->speed.z = (z - work->pos.z) / steps;
    state->steps = steps;
    work->mode = CHAOS4_SLIDE;
    VARS(work)->timer = 0;
    VARS(work)->frame = 0.0F;
}

void event_chaos4_puddle(void)
{
    struct task_work *work = event_chaos4_task->work;
    struct chaos4_state *state = VARS(work)->state;

    state->lift = -event_chaos4_depth;
    state->form.x = 1.0F;
    state->form.y = 1.0F;
    state->form.z = 1.0F;
    state->alpha = 0.6F;
    work->mode = CHAOS4_PUDDLE;
}

static void event_chaos4_init(struct task *task)
{
    task->step = event_chaos4_step;
    task->draw = event_chaos4_draw;
    task->destructor = event_chaos4_destroy;
}

static void event_chaos4_step(struct task *task)
{
    struct task_work *work = task->work;
    struct chaos4_state *state = VARS(work)->state;
    /* @hack spelling: the action through a local, which SHC keeps in R12
     * through the step (MOV.L at 0x0cb8303c in EV000B); without it, the code
     * runs 4 bytes short and 69 of the step's 256 words differ */
    NJS_ACTION *action = &event_chaos4_action;

    event_weld(event_chaos4_welds, action, VARS(work)->frame, CHAOS4_NODES,
               PLAYER_WELDING);
    switch (work->mode) {
    case CHAOS4_STAND:
        VARS(work)->frame += state->action_speed;
        if (VARS(work)->frame >= action->motion->nbFrame) {
            VARS(work)->frame = 0.0F;
        }
        break;
    case CHAOS4_MELT:
        VARS(work)->frame += state->action_speed;
        if (VARS(work)->frame >= action->motion->nbFrame) {
            VARS(work)->frame = 0.0F;
        }
        if (++VARS(work)->timer > state->form_steps) {
            work->mode = CHAOS4_PUDDLE;
            VARS(work)->timer = 0;
            state->form.x = 1.0F;
            state->form.y = 1.0F;
            state->form.z = 1.0F;
        }
        state->lift -= state->lift_speed;
        state->form.x += state->form_speed.x;
        state->form.y += state->form_speed.y;
        state->form.z += state->form_speed.z;
        state->alpha += state->alpha_speed;
        break;
    case CHAOS4_RISE:
        VARS(work)->frame += state->action_speed;
        if (VARS(work)->frame >= action->motion->nbFrame) {
            VARS(work)->frame = 0.0F;
        }
        if (++VARS(work)->timer > state->form_steps) {
            work->mode = CHAOS4_STAND;
            VARS(work)->timer = 0;
            state->form.x = 0.8F;
            state->form.y = 0.01F;
            state->form.z = 0.8F;
            state->lift = 0.0F;
        }
        state->lift += state->lift_speed;
        state->form.x -= state->form_speed.x;
        state->form.y -= state->form_speed.y;
        state->form.z -= state->form_speed.z;
        state->alpha -= state->alpha_speed;
        break;
    case CHAOS4_PUDDLE:
        state->wobble += NJM_DEG_ANG(1);
        break;
    case CHAOS4_SLIDE:
        work->pos.x += state->speed.x;
        work->pos.y += state->speed.y;
        work->pos.z += state->speed.z;
        if (++VARS(work)->timer > state->steps) {
            work->mode = CHAOS4_PUDDLE;
            VARS(work)->frame = 0.0F;
        }
        state->wobble += NJM_DEG_ANG(2);
        break;
    }
    event_chaos4_draw(task);
}

static void event_chaos4_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct chaos4_state *state = VARS(work)->state;
    NJS_POINT3 shadow;
    struct angles slope;

    /* @bug mismatched-call: the draw reads ground_below's height from R0 as an
     * int (LDS R0,FPUL and FLOAT at 0x0cb832cc and 0x0cb832d8 in EV000B),
     * though ground_below returns it in FR0. the shadow's height is whatever R0
     * holds, negated, plus SHADOW_LIFT */
    shadow.y = ground_below_int(work->pos.x, work->pos.y, work->pos.z, &slope);
    shadow.x = work->pos.x;
    shadow.y = -shadow.y;
    shadow.y += SHADOW_LIFT;
    shadow.z = work->pos.z;
    shadow_draw_at(&slope, &shadow, 1.5F);
    draw_set_palette(DRAW_PLAYER_PALETTE);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njTranslate(NULL, 0.0F, 2.0F, 0.0F);
    switch (work->mode) {
    case CHAOS4_STAND:
        njSetTexture(&event_chaos4_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, state->lift + (event_chaos4_depth * 0.5F),
                    0.0F);
        draw_action(&event_chaos4_action, VARS(work)->frame);
        njPopMatrix(1);
        break;
    case CHAOS4_MELT:
        njSetTexture(&event_chaos4_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, state->lift + (event_chaos4_depth * 0.5F),
                    0.0F);
        draw_action(&event_chaos4_action, VARS(work)->frame);
        njPopMatrix(1);
        njSetTexture(&event_chaos0_water_texlist);
        njPushMatrix(NULL);
        njScaleV(NULL, &state->form);
        njScaleV(NULL, &state->puddle_scale);
        draw_tree(event_chaos0_puddle_object);
        njPopMatrix(1);
        break;
    case CHAOS4_RISE:
        njSetTexture(&event_chaos4_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, state->lift + (event_chaos4_depth * 0.5F),
                    0.0F);
        draw_action(&event_chaos4_action, VARS(work)->frame);
        njPopMatrix(1);
        njSetTexture(&event_chaos0_water_texlist);
        njPushMatrix(NULL);
        njScaleV(NULL, &state->form);
        njScaleV(NULL, &state->puddle_scale);
        draw_tree(event_chaos0_puddle_object);
        njPopMatrix(1);
        break;
    case CHAOS4_PUDDLE:
        njSetTexture(&event_chaos0_water_texlist);
        njPushMatrix(NULL);
        njScaleV(NULL, &state->puddle_scale);
        njScale(NULL, 1.0F + (CHAOS4_SWAY * njSin(state->wobble)), 1.0F,
                1.0F + (CHAOS4_SWAY * njCos(state->wobble)));
        draw_tree(event_chaos0_puddle_object);
        njPopMatrix(1);
        break;
    case CHAOS4_SLIDE:
        njSetTexture(&event_chaos0_water_texlist);
        njPushMatrix(NULL);
        njScaleV(NULL, &state->puddle_scale);
        njScale(NULL, 1.0F + (CHAOS4_SWAY * njSin(state->wobble)), 1.0F,
                1.0F + (CHAOS4_SWAY * njCos(state->wobble)));
        draw_tree(event_chaos0_puddle_object);
        njPopMatrix(1);
        break;
    }
    njPopMatrix(1);
    draw_set_palette(0);
}

static void event_chaos4_destroy(struct task *task)
{
    struct task_work *work = task->work;

    event_weld(event_chaos4_welds, &event_chaos4_action, VARS(work)->frame,
               CHAOS4_NODES, PLAYER_WELD_FREE);
    if (VARS(work)->state != NULL) {
        heap_free(VARS(work)->state);
    }
}

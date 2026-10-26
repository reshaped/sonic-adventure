/* Chaos 0 as the events pose him, with welds that close his model's seams
 * in each pose */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "draw/model.h"
#include "player/player.h"
#include "shared/chaos0.h"
#include "shared/chaos0_water_textures.h"
#include "shared/weld.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 10
#include "shared/chaos0/text.inc"

/* the nodes in his model. event_weld poses all of them */
#define CHAOS0_NODES 72
#define CHAOS0_ATTR (NJD_FLAG_USE_ALPHA | NJD_FLAG_IGNORE_SPECULAR)
#define CHAOS0_DEPTH 25.0F /* how far a melt sinks his body */
/* how far above pos his body draws, plus lift, in each mode that draws it but
 * the walk */
#define CHAOS0_BODY_HEIGHT 10.0F
/* the share of the puddle's x and z scale its wobble sways it by */
#define CHAOS0_SWAY 0.125F

/* what his step does, by work->mode. CHAOS0_PLAY7 to CHAOS0_PLAY10 each loop an
 * action of their own. nothing here names the actions, so each mode keeps its
 * number */
enum chaos0_mode {
    CHAOS0_STAND, /* he loops his standing action */
    CHAOS0_WALK, /* speed moves him, drawn walking */
    CHAOS0_HOP, /* speed moves base, and he follows the arc over it */
    CHAOS0_MELT, /* his body sinks as his puddle grows */
    CHAOS0_RISE, /* his body rises as his puddle shrinks */
    CHAOS0_PUDDLE, /* his puddle alone, wobbling in place */
    CHAOS0_SLIDE, /* his puddle alone, wobbling as speed moves it */
    CHAOS0_PLAY7,
    CHAOS0_PLAY8,
    CHAOS0_PLAY9,
    CHAOS0_PLAY10
};

/* his state, which event_chaos0_create allocates and his destructor frees */
struct chaos0_state {
    /* where the last hop, walk or slide is headed. nothing reads it */
    NJS_POINT3 to;
    NJS_POINT3 base; /* the point under the hop's arc */
    NJS_VECTOR speed; /* how far a move takes him each step */
    Float arc; /* how far the hop is along its arc, from 0 to 180 degrees */
    Float arc_speed;
    Float height; /* how high the arc rises over base */
    Sint32 steps; /* the steps speed divides a move's way into */
    Float hop_speed; /* frames of the hop's action played each step */
    Sint32 hop; /* which of the four hops, from 0 */
    Float action_speed; /* frames of his other actions played each step */
    Sint32 form_steps; /* the steps a melt or a rise divides its change into */
    Float lift; /* added to CHAOS0_BODY_HEIGHT */
    Float lift_speed;
    NJS_VECTOR puddle_scale;
    NJS_VECTOR form_speed;
    NJS_VECTOR form; /* the puddle's scale as he melts and rises */
    /* the melt's and the rise's draws pass alpha - 1 to draw_set_material for
     * the puddle */
    Float alpha;
    Float alpha_speed;
    Angle wobble; /* the phase of the wobble that sways the puddle's scale */
};

struct chaos0_vars {
    Float frame; /* his action's frame */
    Sint32 timer; /* steps into the move, the melt or the rise */
    struct chaos0_state *state;
};

#define VARS(work) ((struct chaos0_vars *)&(work)->vars)

/* his actions but play10's, which each event that links chaos0.c defines */
extern NJS_ACTION event_chaos0_walk_action;
/* @hack type: these eight actions declared as arrays. SHC then pools each one's
 * address, and reads a motion 4 bytes past it (MOV.L @(4,R2),R1 at 0x0cb80d2c
 * in EV0002). with the stand action alone declared as one object, the step runs
 * 12 bytes long and 412 of its 512 words differ. with a play action alone so
 * declared, the step runs 4 bytes long and 54 to 61 of its 512 words differ.
 * with the first hop action alone so declared, 3 of each hop function's 90
 * words differ. with any hop action alone so declared, 42 of the draw's 738
 * words differ */
extern NJS_ACTION event_chaos0_stand_action[];
extern NJS_ACTION event_chaos0_hop0_action[];
extern NJS_ACTION event_chaos0_hop1_action[];
extern NJS_ACTION event_chaos0_hop2_action[];
extern NJS_ACTION event_chaos0_hop3_action[];
extern NJS_ACTION event_chaos0_play7_action[];
extern NJS_ACTION event_chaos0_play8_action[];
extern NJS_ACTION event_chaos0_play9_action[];
/* his puddle, his model and the nodes of it that the welds join, which each
 * event defines too. a node's name gives its place in the model's walk, from 0
 * at event_chaos0_object. the walk takes each node, then its child's tree, then
 * its next sibling's */
extern NJS_OBJECT event_chaos0_puddle_object;
extern NJS_OBJECT event_chaos0_object;
extern NJS_OBJECT event_chaos0_node3;
extern NJS_OBJECT event_chaos0_node21;
extern NJS_OBJECT event_chaos0_node23;
extern NJS_OBJECT event_chaos0_node25;
extern NJS_OBJECT event_chaos0_node26;
extern NJS_OBJECT event_chaos0_node32;
extern NJS_OBJECT event_chaos0_node34;
extern NJS_OBJECT event_chaos0_node36;
extern NJS_OBJECT event_chaos0_node37;
extern NJS_OBJECT event_chaos0_node38;
extern NJS_OBJECT event_chaos0_node45;
extern NJS_OBJECT event_chaos0_node46;
extern NJS_OBJECT event_chaos0_node47;
extern NJS_OBJECT event_chaos0_node49;
extern NJS_OBJECT event_chaos0_node50;
extern NJS_OBJECT event_chaos0_node66;
extern NJS_OBJECT event_chaos0_node67;
extern NJS_OBJECT event_chaos0_node68;
extern NJS_OBJECT event_chaos0_node70;
extern NJS_OBJECT event_chaos0_node71;

extern NJS_MDATA2 event_chaos0_play10_mdata[];

static NJS_MOTION event_chaos0_play10_motion = {
#include "shared/chaos0/event_chaos0_play10_motion.inc"
};
/* @hack type: an array of one. the actions above are declared as arrays too. as
 * one NJS_ACTION, the draw runs 4 bytes long. then 4 of the step's 512 words
 * differ, and 678 of the draw's 738 words differ */
static NJS_ACTION event_chaos0_play10_action[1] = {
#include "shared/chaos0/event_chaos0_play10_action.inc"
};
static NJS_TEXNAME event_chaos0_textures[3] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
    {TEXT_3, 0, 0},
};
NJS_TEXLIST event_chaos0_texlist = {
#include "shared/chaos0/event_chaos0_texlist.inc"
};

/* the last Chaos 0 created. nothing clears it */
static struct task *event_chaos0_task = NULL;

/* each weld's vertex pairs: a vertex's index in one node's model, then its
 * index in the other's. a name gives the two nodes */
static Uint16 event_chaos0_indices_37_36[10] = {
#include "shared/chaos0/event_chaos0_indices_37_36.inc"
};
static Uint16 event_chaos0_indices_36_32[10] = {
#include "shared/chaos0/event_chaos0_indices_36_32.inc"
};
static Uint16 event_chaos0_indices_32_34[10] = {
#include "shared/chaos0/event_chaos0_indices_32_34.inc"
};
static Uint16 event_chaos0_indices_26_25[10] = {
#include "shared/chaos0/event_chaos0_indices_26_25.inc"
};
static Uint16 event_chaos0_indices_25_21[10] = {
#include "shared/chaos0/event_chaos0_indices_25_21.inc"
};
static Uint16 event_chaos0_indices_21_23[10] = {
#include "shared/chaos0/event_chaos0_indices_21_23.inc"
};
static Uint16 event_chaos0_indices_71_70[10] = {
#include "shared/chaos0/event_chaos0_indices_71_70.inc"
};
static Uint16 event_chaos0_indices_70_66[10] = {
#include "shared/chaos0/event_chaos0_indices_70_66.inc"
};
static Uint16 event_chaos0_indices_67_68[8] = {
#include "shared/chaos0/event_chaos0_indices_67_68.inc"
};
static Uint16 event_chaos0_indices_50_49[10] = {
#include "shared/chaos0/event_chaos0_indices_50_49.inc"
};
static Uint16 event_chaos0_indices_49_45[10] = {
#include "shared/chaos0/event_chaos0_indices_49_45.inc"
};
static Uint16 event_chaos0_indices_46_47[8] = {
#include "shared/chaos0/event_chaos0_indices_46_47.inc"
};
static Uint16 event_chaos0_indices_38_3[12] = {
#include "shared/chaos0/event_chaos0_indices_38_3.inc"
};
static Uint16 event_chaos0_indices_38_37[10] = {
#include "shared/chaos0/event_chaos0_indices_38_37.inc"
};
static Uint16 event_chaos0_indices_38_26[10] = {
#include "shared/chaos0/event_chaos0_indices_38_26.inc"
};
static Uint16 event_chaos0_indices_66_67[10] = {
#include "shared/chaos0/event_chaos0_indices_66_67.inc"
};
static Uint16 event_chaos0_indices_45_46[10] = {
#include "shared/chaos0/event_chaos0_indices_45_46.inc"
};

static struct weld event_chaos0_welds[] = {
    {&event_chaos0_object, &event_chaos0_node37, &event_chaos0_node36, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos0_indices_37_36},
    {&event_chaos0_object, &event_chaos0_node36, &event_chaos0_node32, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos0_indices_36_32},
    {&event_chaos0_object, &event_chaos0_node32, &event_chaos0_node34, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos0_indices_32_34},
    {&event_chaos0_object, &event_chaos0_node26, &event_chaos0_node25, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos0_indices_26_25},
    {&event_chaos0_object, &event_chaos0_node25, &event_chaos0_node21, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos0_indices_25_21},
    {&event_chaos0_object, &event_chaos0_node21, &event_chaos0_node23, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos0_indices_21_23},
    {&event_chaos0_object, &event_chaos0_node71, &event_chaos0_node70, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos0_indices_71_70},
    {&event_chaos0_object, &event_chaos0_node70, &event_chaos0_node66, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos0_indices_70_66},
    {&event_chaos0_object, &event_chaos0_node67, &event_chaos0_node68, 4,
     WELD_TO_A, 0, 0, NULL, event_chaos0_indices_67_68},
    {&event_chaos0_object, &event_chaos0_node50, &event_chaos0_node49, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos0_indices_50_49},
    {&event_chaos0_object, &event_chaos0_node49, &event_chaos0_node45, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos0_indices_49_45},
    {&event_chaos0_object, &event_chaos0_node46, &event_chaos0_node47, 4,
     WELD_TO_A, 0, 0, NULL, event_chaos0_indices_46_47},
    {&event_chaos0_object, &event_chaos0_node38, &event_chaos0_node3, 6,
     WELD_TO_B, 0, 0, NULL, event_chaos0_indices_38_3},
    {&event_chaos0_object, &event_chaos0_node38, &event_chaos0_node37, 5,
     WELD_TO_B, 0, 0, NULL, event_chaos0_indices_38_37},
    {&event_chaos0_object, &event_chaos0_node38, &event_chaos0_node26, 5,
     WELD_TO_B, 0, 0, NULL, event_chaos0_indices_38_26},
    {&event_chaos0_object, &event_chaos0_node66, &event_chaos0_node67, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos0_indices_66_67},
    {&event_chaos0_object, &event_chaos0_node45, &event_chaos0_node46, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos0_indices_45_46},
    {NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL},
};

static void event_chaos0_init(struct task *task);
static void event_chaos0_step(struct task *task);
static void event_chaos0_draw(struct task *task);
static void event_chaos0_destroy(struct task *task);

void event_chaos0_load_textures(void)
{
    texture_load_pvm("CHAOS0", &event_chaos0_texlist);
    njLoadTexture(&event_chaos0_water_texlist);
}

void event_chaos0_release_textures(void)
{
    njReleaseTexture(&event_chaos0_texlist);
    njReleaseTexture(&event_chaos0_water_texlist);
}

void event_chaos0_create(Float x, Float y, Float z, Angle ang_x, Angle ang_y,
                         Angle ang_z, int steps)
{
    struct chaos0_state *state;
    struct task_work *work;

    event_chaos0_task = task_create(TASK_WORK, 5, event_chaos0_init);
    state = heap_alloc(sizeof(struct chaos0_state));
    work = event_chaos0_task->work;
    VARS(work)->state = state;
    work->mode = CHAOS0_STAND;
    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    work->ang.x = ang_x;
    work->ang.y = ang_y;
    work->ang.z = ang_z;
    work->scl.x = 1.0F;
    work->scl.y = 1.0F;
    work->scl.z = 1.0F;
    VARS(work)->frame = 0.0F;
    state->action_speed = 1.0F;
    state->form_steps = steps;
    state->lift = 0.0F;
    state->lift_speed = CHAOS0_DEPTH / (Float)state->form_steps;
    state->puddle_scale.x = 2.0F;
    state->puddle_scale.y = 1.0F;
    state->puddle_scale.z = 2.0F;
    state->form_speed.x = 0.2F / (Float)state->form_steps;
    state->form_speed.y = 1.0F / (Float)state->form_steps;
    state->form_speed.z = 0.2F / (Float)state->form_steps;
    state->form.x = 0.8F;
    state->form.y = 0.01F;
    state->form.z = 0.8F;
    state->alpha = 0.05F;
    state->alpha_speed = 0.5F / (Float)state->form_steps;
    state->wobble = 0;
    event_weld(event_chaos0_welds, event_chaos0_stand_action, VARS(work)->frame,
               CHAOS0_NODES, PLAYER_WELD_MAKE);
}

void event_chaos0_kill(void)
{
    if (event_chaos0_task != NULL) {
        task_kill(event_chaos0_task);
    }
}

void event_chaos0_hop0(Float x, Float y, Float z, Float height, int steps)
{
    struct task_work *work = event_chaos0_task->work;
    struct chaos0_state *state = VARS(work)->state;

    state->to.x = x;
    state->to.y = y;
    state->to.z = z;
    state->base = work->pos;
    state->speed.x = (x - work->pos.x) / (Float)steps;
    state->speed.y = (y - work->pos.y) / (Float)steps;
    state->speed.z = (z - work->pos.z) / (Float)steps;
    state->arc = 0.0F;
    state->arc_speed = 180.0F / (Float)steps;
    state->height = height;
    state->steps = steps;
    state->hop_speed =
        (Float)event_chaos0_hop0_action->motion->nbFrame / (Float)steps;
    state->hop = 0;
    work->ang.y =
        (Angle)(atan2f(state->speed.z, -state->speed.x) * ANGLE_PER_RADIAN);
    work->mode = CHAOS0_HOP;
    VARS(work)->timer = 0;
    VARS(work)->frame = 0.0F;
}

void event_chaos0_hop1(Float x, Float y, Float z, Float height, int steps)
{
    struct task_work *work = event_chaos0_task->work;
    struct chaos0_state *state = VARS(work)->state;

    state->to.x = x;
    state->to.y = y;
    state->to.z = z;
    state->base = work->pos;
    state->speed.x = (x - work->pos.x) / (Float)steps;
    state->speed.y = (y - work->pos.y) / (Float)steps;
    state->speed.z = (z - work->pos.z) / (Float)steps;
    state->arc = 0.0F;
    state->arc_speed = 180.0F / (Float)steps;
    state->height = height;
    state->steps = steps;
    state->hop_speed =
        (Float)event_chaos0_hop0_action->motion->nbFrame / (Float)steps;
    state->hop = 1;
    work->ang.y =
        (Angle)(atan2f(state->speed.z, -state->speed.x) * ANGLE_PER_RADIAN);
    work->mode = CHAOS0_HOP;
    VARS(work)->timer = 0;
    VARS(work)->frame = 0.0F;
}

void event_chaos0_hop2(Float x, Float y, Float z, Float height, int steps)
{
    struct task_work *work = event_chaos0_task->work;
    struct chaos0_state *state = VARS(work)->state;

    state->to.x = x;
    state->to.y = y;
    state->to.z = z;
    state->base = work->pos;
    state->speed.x = (x - work->pos.x) / (Float)steps;
    state->speed.y = (y - work->pos.y) / (Float)steps;
    state->speed.z = (z - work->pos.z) / (Float)steps;
    state->arc = 0.0F;
    state->arc_speed = 180.0F / (Float)steps;
    state->height = height;
    state->steps = steps;
    state->hop_speed =
        (Float)event_chaos0_hop0_action->motion->nbFrame / (Float)steps;
    state->hop = 2;
    work->ang.y =
        (Angle)(atan2f(state->speed.z, -state->speed.x) * ANGLE_PER_RADIAN);
    work->mode = CHAOS0_HOP;
    VARS(work)->timer = 0;
    VARS(work)->frame = 0.0F;
}

void event_chaos0_hop3(Float x, Float y, Float z, Float height, int steps)
{
    struct task_work *work = event_chaos0_task->work;
    struct chaos0_state *state = VARS(work)->state;

    state->to.x = x;
    state->to.y = y;
    state->to.z = z;
    state->base = work->pos;
    state->speed.x = (x - work->pos.x) / (Float)steps;
    state->speed.y = (y - work->pos.y) / (Float)steps;
    state->speed.z = (z - work->pos.z) / (Float)steps;
    state->arc = 0.0F;
    state->arc_speed = 180.0F / (Float)steps;
    state->height = height;
    state->steps = steps;
    state->hop_speed =
        (Float)event_chaos0_hop0_action->motion->nbFrame / (Float)steps;
    state->hop = 3;
    work->ang.y =
        (Angle)(atan2f(state->speed.z, -state->speed.x) * ANGLE_PER_RADIAN);
    work->mode = CHAOS0_HOP;
    VARS(work)->timer = 0;
    VARS(work)->frame = 0.0F;
}

void event_chaos0_walk(Float x, Float y, Float z, int steps)
{
    struct task_work *work = event_chaos0_task->work;
    struct chaos0_state *state = VARS(work)->state;

    state->to.x = x;
    state->to.y = y;
    state->to.z = z;
    state->speed.x = (x - work->pos.x) / (Float)steps;
    state->speed.y = (y - work->pos.y) / (Float)steps;
    state->speed.z = (z - work->pos.z) / (Float)steps;
    state->steps = steps;
    work->ang.y =
        (Angle)(atan2f(state->speed.z, -state->speed.x) * ANGLE_PER_RADIAN);
    work->mode = CHAOS0_WALK;
    VARS(work)->timer = 0;
    VARS(work)->frame = 0.0F;
}

void event_chaos0_slide(Float x, Float y, Float z, int steps)
{
    struct task_work *work = event_chaos0_task->work;
    struct chaos0_state *state = VARS(work)->state;

    state->to.x = x;
    state->to.y = y;
    state->to.z = z;
    state->speed.x = (x - work->pos.x) / (Float)steps;
    state->speed.y = (y - work->pos.y) / (Float)steps;
    state->speed.z = (z - work->pos.z) / (Float)steps;
    state->steps = steps;
    work->mode = CHAOS0_SLIDE;
    VARS(work)->timer = 0;
    VARS(work)->frame = 0.0F;
}

struct task *event_chaos0_get_task(void)
{
    return event_chaos0_task;
}

void event_chaos0_set_speed(Float speed)
{
    struct task_work *work = event_chaos0_task->work;
    struct chaos0_state *state = VARS(work)->state;

    state->action_speed = speed;
}

void event_chaos0_melt(void)
{
    struct task_work *work = event_chaos0_task->work;

    VARS(work)->timer = 0;
    work->mode = CHAOS0_MELT;
}

void event_chaos0_rise(void)
{
    struct task_work *work = event_chaos0_task->work;

    VARS(work)->timer = 0;
    work->mode = CHAOS0_RISE;
}

void event_chaos0_set_pos(Float x, Float y, Float z)
{
    struct task_work *work = event_chaos0_task->work;

    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
}

void event_chaos0_set_ang(Angle x, Angle y, Angle z)
{
    struct task_work *work = event_chaos0_task->work;

    work->ang.x = x;
    work->ang.y = y;
    work->ang.z = z;
}

void event_chaos0_puddle(void)
{
    struct task_work *work = event_chaos0_task->work;
    struct chaos0_state *state = VARS(work)->state;

    state->lift = -CHAOS0_DEPTH;
    state->form.x = 1.0F;
    state->form.y = 1.0F;
    state->form.z = 1.0F;
    state->alpha = 0.5F;
    work->mode = CHAOS0_PUDDLE;
}

void event_chaos0_set_scale(Float x, Float y, Float z)
{
    struct task_work *work = event_chaos0_task->work;

    work->scl.x = x;
    work->scl.y = y;
    work->scl.z = z;
}

void event_chaos0_stand(void)
{
    struct task_work *work = event_chaos0_task->work;

    work->mode = CHAOS0_STAND;
}

void event_chaos0_play7(void)
{
    struct task_work *work = event_chaos0_task->work;

    work->mode = CHAOS0_PLAY7;
}

void event_chaos0_play8(void)
{
    struct task_work *work = event_chaos0_task->work;

    work->mode = CHAOS0_PLAY8;
}

void event_chaos0_play9(void)
{
    struct task_work *work = event_chaos0_task->work;

    work->mode = CHAOS0_PLAY9;
}

void event_chaos0_play10(void)
{
    struct task_work *work = event_chaos0_task->work;

    work->mode = CHAOS0_PLAY10;
}

static void event_chaos0_init(struct task *task)
{
    task->step = event_chaos0_step;
    task->draw = event_chaos0_draw;
    task->destructor = event_chaos0_destroy;
}

static void event_chaos0_step(struct task *task)
{
    struct task_work *work = task->work;
    struct chaos0_state *state = VARS(work)->state;

    switch (work->mode) {
    case CHAOS0_STAND:
        event_weld(event_chaos0_welds, event_chaos0_stand_action,
                   VARS(work)->frame, CHAOS0_NODES, PLAYER_WELDING);
        VARS(work)->frame += state->action_speed;
        if (VARS(work)->frame >=
            (Float)event_chaos0_stand_action->motion->nbFrame) {
            VARS(work)->frame = 0.0F;
        }
        break;
    case CHAOS0_WALK:
        /* the welds take the stand action's pose, though the draw plays the
         * walk action. frame wraps at the stand action's end */
        event_weld(event_chaos0_welds, event_chaos0_stand_action,
                   VARS(work)->frame, CHAOS0_NODES, PLAYER_WELDING);
        work->pos.x += state->speed.x;
        work->pos.y += state->speed.y;
        work->pos.z += state->speed.z;
        if (++VARS(work)->timer > state->steps) {
            work->mode = CHAOS0_STAND;
            VARS(work)->frame = 0.0F;
        }
        VARS(work)->frame += state->action_speed;
        if (VARS(work)->frame >=
            (Float)event_chaos0_stand_action->motion->nbFrame) {
            VARS(work)->frame = 0.0F;
        }
        break;
    case CHAOS0_SLIDE:
        work->pos.x += state->speed.x;
        work->pos.y += state->speed.y;
        work->pos.z += state->speed.z;
        if (++VARS(work)->timer > state->steps) {
            work->mode = CHAOS0_PUDDLE;
            VARS(work)->frame = 0.0F;
        }
        state->wobble += NJM_DEG_ANG(2);
        break;
    case CHAOS0_HOP:
        state->base.x += state->speed.x;
        state->base.y += state->speed.y;
        state->base.z += state->speed.z;
        work->pos.x = state->base.x;
        work->pos.y =
            state->base.y +
            (njSin((Angle)(state->arc * ANGLE_PER_DEGREE)) * state->height);
        work->pos.z = state->base.z;
        state->arc += state->arc_speed;
        if (++VARS(work)->timer > state->steps) {
            work->mode = CHAOS0_STAND;
            VARS(work)->frame = 0.0F;
        }
        VARS(work)->frame += state->hop_speed;
        break;
    case CHAOS0_MELT:
        event_weld(event_chaos0_welds, event_chaos0_stand_action,
                   VARS(work)->frame, CHAOS0_NODES, PLAYER_WELDING);
        VARS(work)->frame += state->action_speed;
        if (VARS(work)->frame >=
            (Float)event_chaos0_stand_action->motion->nbFrame) {
            VARS(work)->frame = 0.0F;
        }
        if (++VARS(work)->timer > state->form_steps) {
            work->mode = CHAOS0_PUDDLE;
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
    case CHAOS0_RISE:
        event_weld(event_chaos0_welds, event_chaos0_stand_action,
                   VARS(work)->frame, CHAOS0_NODES, PLAYER_WELDING);
        VARS(work)->frame += state->action_speed;
        if (VARS(work)->frame >=
            (Float)event_chaos0_stand_action->motion->nbFrame) {
            VARS(work)->frame = 0.0F;
        }
        if (++VARS(work)->timer > state->form_steps) {
            work->mode = CHAOS0_STAND;
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
    case CHAOS0_PUDDLE:
        state->wobble += NJM_DEG_ANG(1);
        break;
    case CHAOS0_PLAY7:
        event_weld(event_chaos0_welds, event_chaos0_play7_action,
                   VARS(work)->frame, CHAOS0_NODES, PLAYER_WELDING);
        VARS(work)->frame += state->action_speed;
        if (VARS(work)->frame >=
            (Float)event_chaos0_play7_action->motion->nbFrame) {
            VARS(work)->frame = 0.0F;
        }
        break;
    case CHAOS0_PLAY8:
        event_weld(event_chaos0_welds, event_chaos0_play8_action,
                   VARS(work)->frame, CHAOS0_NODES, PLAYER_WELDING);
        VARS(work)->frame += state->action_speed;
        if (VARS(work)->frame >=
            (Float)event_chaos0_play8_action->motion->nbFrame) {
            VARS(work)->frame = 0.0F;
        }
        break;
    case CHAOS0_PLAY9:
        event_weld(event_chaos0_welds, event_chaos0_play9_action,
                   VARS(work)->frame, CHAOS0_NODES, PLAYER_WELDING);
        VARS(work)->frame += state->action_speed;
        if (VARS(work)->frame >=
            (Float)event_chaos0_play9_action->motion->nbFrame) {
            VARS(work)->frame = 0.0F;
        }
        break;
    case CHAOS0_PLAY10:
        event_weld(event_chaos0_welds, event_chaos0_play10_action,
                   VARS(work)->frame, CHAOS0_NODES, PLAYER_WELDING);
        VARS(work)->frame += state->action_speed;
        if (VARS(work)->frame >=
            (Float)event_chaos0_play10_action->motion->nbFrame) {
            VARS(work)->frame = 0.0F;
        }
        break;
    }
    event_chaos0_draw(task);
}

static void event_chaos0_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct chaos0_state *state = VARS(work)->state;
    Angle ang;
    /* @hack redundant: 24 bytes of frame that no code reads, of a type unknown,
     * declared as angles and a point. the image's ADD #-28,R15 reserves them
     * (0x0cb8163a in EV0002); without them, 2 of 738 words differ */
    struct angles unused_ang;
    NJS_POINT3 unused_pos;

    njSetTexture(&event_chaos0_texlist);
    draw_set_palette(DRAW_PLAYER_PALETTE);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    switch (work->mode) {
    case CHAOS0_STAND:
        draw_attr_save();
        draw_attr_set(0, CHAOS0_ATTR);
        draw_set_material(0.1F, 0.0F, 0.0F, 0.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, state->lift + CHAOS0_BODY_HEIGHT, 0.0F);
        njScaleV(NULL, &work->scl);
        draw_action(event_chaos0_stand_action, VARS(work)->frame);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_attr_restore();
        break;
    case CHAOS0_PLAY7:
        draw_attr_save();
        draw_attr_set(0, CHAOS0_ATTR);
        draw_set_material(0.1F, 0.0F, 0.0F, 0.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, state->lift + CHAOS0_BODY_HEIGHT, 0.0F);
        njScaleV(NULL, &work->scl);
        draw_action(event_chaos0_play7_action, VARS(work)->frame);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_attr_restore();
        break;
    case CHAOS0_PLAY8:
        draw_attr_save();
        draw_attr_set(0, CHAOS0_ATTR);
        draw_set_material(0.1F, 0.0F, 0.0F, 0.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, state->lift + CHAOS0_BODY_HEIGHT, 0.0F);
        njScaleV(NULL, &work->scl);
        draw_action(event_chaos0_play8_action, VARS(work)->frame);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_attr_restore();
        break;
    case CHAOS0_PLAY9:
        draw_attr_save();
        draw_attr_set(0, CHAOS0_ATTR);
        draw_set_material(0.1F, 0.0F, 0.0F, 0.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, state->lift + CHAOS0_BODY_HEIGHT, 0.0F);
        njScaleV(NULL, &work->scl);
        draw_action(event_chaos0_play9_action, VARS(work)->frame);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_attr_restore();
        break;
    case CHAOS0_PLAY10:
        draw_attr_save();
        draw_attr_set(0, CHAOS0_ATTR);
        draw_set_material(0.1F, 0.0F, 0.0F, 0.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, state->lift + CHAOS0_BODY_HEIGHT, 0.0F);
        njScaleV(NULL, &work->scl);
        draw_action(event_chaos0_play10_action, VARS(work)->frame);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_attr_restore();
        break;
    case CHAOS0_WALK:
        draw_attr_save();
        draw_attr_set(0, CHAOS0_ATTR);
        draw_set_material(-0.5F, 0.0F, 0.0F, 0.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        draw_action(&event_chaos0_walk_action, VARS(work)->frame);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_attr_restore();
        break;
    case CHAOS0_SLIDE:
        njSetTexture(&event_chaos0_water_texlist);
        draw_attr_save();
        draw_attr_set(0, CHAOS0_ATTR);
        draw_set_material(-0.3F, 0.0F, 0.0F, 0.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        njPushMatrix(NULL);
        njScaleV(NULL, &state->puddle_scale);
        njScale(NULL, 1.0F + (njSin(state->wobble) * CHAOS0_SWAY), 1.0F,
                1.0F + (njCos(state->wobble) * CHAOS0_SWAY));
        njDrawObject(&event_chaos0_puddle_object);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_attr_restore();
        break;
    case CHAOS0_HOP:
        draw_attr_save();
        draw_attr_set(0, CHAOS0_ATTR);
        draw_set_material(0.1F, 0.0F, 0.0F, 0.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        switch (state->hop) {
        case 0:
            event_weld(event_chaos0_welds, event_chaos0_hop0_action,
                       VARS(work)->frame, CHAOS0_NODES, PLAYER_WELDING);
            njPushMatrix(NULL);
            njTranslate(NULL, 0.0F, state->lift + CHAOS0_BODY_HEIGHT, 0.0F);
            draw_action(event_chaos0_hop0_action, VARS(work)->frame);
            njPopMatrix(1);
            break;
        case 1:
            event_weld(event_chaos0_welds, event_chaos0_hop1_action,
                       VARS(work)->frame, CHAOS0_NODES, PLAYER_WELDING);
            njPushMatrix(NULL);
            njTranslate(NULL, 0.0F, state->lift + CHAOS0_BODY_HEIGHT, 0.0F);
            draw_action(event_chaos0_hop1_action, VARS(work)->frame);
            njPopMatrix(1);
            break;
        case 2:
            event_weld(event_chaos0_welds, event_chaos0_hop2_action,
                       VARS(work)->frame, CHAOS0_NODES, PLAYER_WELDING);
            njPushMatrix(NULL);
            njTranslate(NULL, 0.0F, state->lift + CHAOS0_BODY_HEIGHT, 0.0F);
            draw_action(event_chaos0_hop2_action, VARS(work)->frame);
            njPopMatrix(1);
            break;
        case 3:
            event_weld(event_chaos0_welds, event_chaos0_hop3_action,
                       VARS(work)->frame, CHAOS0_NODES, PLAYER_WELDING);
            njPushMatrix(NULL);
            njTranslate(NULL, 0.0F, state->lift + CHAOS0_BODY_HEIGHT, 0.0F);
            draw_action(event_chaos0_hop3_action, VARS(work)->frame);
            njPopMatrix(1);
            break;
        }
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_attr_restore();
        break;
    /* @hack spelling: CHAOS0_MELT's and CHAOS0_RISE's cases kept apart, with
     * the same draw in each, as EV0002 has code for each at 0x0cb81a84 and
     * 0x0cb81b3c. as one case, the draw runs 180 bytes short and 499 of its
     * 738 words differ */
    case CHAOS0_MELT:
        draw_attr_save();
        draw_attr_set(0, CHAOS0_ATTR);
        draw_set_material(0.1F, 0.0F, 0.0F, 0.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        njSetTexture(&event_chaos0_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, state->lift + CHAOS0_BODY_HEIGHT, 0.0F);
        draw_action(event_chaos0_stand_action, VARS(work)->frame);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_attr_restore();
        njSetTexture(&event_chaos0_water_texlist);
        draw_attr_save();
        draw_attr_set(0, CHAOS0_ATTR);
        draw_set_material(state->alpha - 1.0F, 0.0F, 0.0F, 0.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        njPushMatrix(NULL);
        njScaleV(NULL, &state->form);
        njScaleV(NULL, &state->puddle_scale);
        njDrawObject(&event_chaos0_puddle_object);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_attr_restore();
        break;
    case CHAOS0_RISE:
        draw_attr_save();
        draw_attr_set(0, CHAOS0_ATTR);
        draw_set_material(0.1F, 0.0F, 0.0F, 0.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        njSetTexture(&event_chaos0_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, state->lift + CHAOS0_BODY_HEIGHT, 0.0F);
        draw_action(event_chaos0_stand_action, VARS(work)->frame);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_attr_restore();
        njSetTexture(&event_chaos0_water_texlist);
        draw_attr_save();
        draw_attr_set(0, CHAOS0_ATTR);
        draw_set_material(state->alpha - 1.0F, 0.0F, 0.0F, 0.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        njPushMatrix(NULL);
        njScaleV(NULL, &state->form);
        njScaleV(NULL, &state->puddle_scale);
        njDrawObject(&event_chaos0_puddle_object);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_attr_restore();
        break;
    case CHAOS0_PUDDLE:
        njSetTexture(&event_chaos0_water_texlist);
        draw_attr_save();
        draw_attr_set(0, CHAOS0_ATTR);
        draw_set_material(-0.3F, 0.0F, 0.0F, 0.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        njPushMatrix(NULL);
        njScaleV(NULL, &state->puddle_scale);
        njScale(NULL, 1.0F + (njSin(state->wobble) * CHAOS0_SWAY), 1.0F,
                1.0F + (njCos(state->wobble) * CHAOS0_SWAY));
        njDrawObject(&event_chaos0_puddle_object);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_attr_restore();
        break;
    }
    njPopMatrix(1);
    draw_set_palette(0);
}

static void event_chaos0_destroy(struct task *task)
{
    struct task_work *work = task->work;

    event_weld(event_chaos0_welds, event_chaos0_stand_action, VARS(work)->frame,
               CHAOS0_NODES, PLAYER_WELD_FREE);
    if (VARS(work)->state != NULL) {
        heap_free(VARS(work)->state);
    }
}

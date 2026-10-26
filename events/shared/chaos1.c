/* Chaos 1 as the events pose him, with welds that close his model's seams
 * in each pose */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "player/player.h"
#include "shared/chaos0_water_textures.h"
#include "shared/chaos1.h"
#include "shared/chaos1_model.h"
#include "shared/weld.h"
#include "system/task.h"
#include "system/util.h"

/* event_weld's nodes: more than his model's 72, so it poses them all */
#define CHAOS1_NODES 92
#define CHAOS1_DEPTH 30.0F /* how far a melt sinks his body */
/* how far above the puddle his body draws, plus lift */
#define CHAOS1_BODY_HEIGHT 10.0F
/* the share of the puddle's x and z scale its wobble sways it by */
#define CHAOS1_SWAY 0.125F

/* what his step does, by work->mode */
enum chaos1_mode {
    CHAOS1_STAND, /* he loops his action */
    CHAOS1_MELT, /* his body sinks as the puddle grows */
    CHAOS1_RISE, /* his body rises as the puddle shrinks */
    CHAOS1_PUDDLE /* the puddle alone, wobbling in place */
};

/* his state, which event_chaos1_create allocates and his destructor frees */
struct chaos1_state {
    Sint32 form_steps; /* the steps a melt or a rise divides its change into */
    Float action_speed; /* how many frames of his action play each step */
    Float lift; /* added to CHAOS1_BODY_HEIGHT */
    Float lift_speed;
    NJS_VECTOR puddle_scale;
    NJS_VECTOR form_speed;
    NJS_VECTOR form; /* the puddle's scale as he melts and rises */
    /* the puddle's alpha as he melts and rises. no draw reads it */
    Float alpha;
    Float alpha_speed;
};

struct chaos1_vars {
    Float frame; /* his action's frame */
    Sint32 timer; /* steps into a melt, a rise or the puddle */
    struct chaos1_state *state;
};

#define VARS(work) ((struct chaos1_vars *)&(work)->vars)

/* each event that links chaos1.c defines these, and loads the texlist from
 * EV_CHAOS1 */
extern NJS_TEXLIST event_chaos1_texlist;
extern NJS_OBJECT event_chaos0_puddle_object;
extern NJS_MDATA3 event_chaos1_mdata[];

static NJS_MOTION event_chaos1_motion = {
#include "shared/chaos1/event_chaos1_motion.inc"
};
/* @hack type: an array of one. SHC then keeps its address in R12 through the
 * step, and reads the motion 4 bytes past it (0x0cb838fe and 0x0cb8393e in
 * EV0008). as one NJS_ACTION, 62 of the step's 216 words differ, and 96 of the
 * draw's 235 words differ */
static NJS_ACTION event_chaos1_action[1] = {
#include "shared/chaos1/event_chaos1_action.inc"
};
/* the last Chaos 1 created. nothing clears it */
static struct task *event_chaos1_task = NULL;

/* each weld's vertex pairs: a vertex's index in one part's model, then its
 * index in the other's. a name gives the joint the two parts meet at */
static Uint16 event_chaos1_elbow_plus_z_indices[10] = {
#include "shared/chaos1/event_chaos1_elbow_plus_z_indices.inc"
};
static Uint16 event_chaos1_wrist_plus_z_indices[10] = {
#include "shared/chaos1/event_chaos1_wrist_plus_z_indices.inc"
};
static Uint16 event_chaos1_knuckles_plus_z_indices[10] = {
#include "shared/chaos1/event_chaos1_knuckles_plus_z_indices.inc"
};
static Uint16 event_chaos1_elbow_minus_z_indices[10] = {
#include "shared/chaos1/event_chaos1_elbow_minus_z_indices.inc"
};
static Uint16 event_chaos1_wrist_minus_z_indices[10] = {
#include "shared/chaos1/event_chaos1_wrist_minus_z_indices.inc"
};
static Uint16 event_chaos1_knuckles_minus_z_indices[10] = {
#include "shared/chaos1/event_chaos1_knuckles_minus_z_indices.inc"
};
static Uint16 event_chaos1_knee_plus_z_indices[10] = {
#include "shared/chaos1/event_chaos1_knee_plus_z_indices.inc"
};
static Uint16 event_chaos1_ankle_plus_z_indices[10] = {
#include "shared/chaos1/event_chaos1_ankle_plus_z_indices.inc"
};
static Uint16 event_chaos1_toes_plus_z_indices[8] = {
#include "shared/chaos1/event_chaos1_toes_plus_z_indices.inc"
};
static Uint16 event_chaos1_knee_minus_z_indices[10] = {
#include "shared/chaos1/event_chaos1_knee_minus_z_indices.inc"
};
static Uint16 event_chaos1_ankle_minus_z_indices[10] = {
#include "shared/chaos1/event_chaos1_ankle_minus_z_indices.inc"
};
static Uint16 event_chaos1_toes_minus_z_indices[8] = {
#include "shared/chaos1/event_chaos1_toes_minus_z_indices.inc"
};
static Uint16 event_chaos1_waist_indices[12] = {
#include "shared/chaos1/event_chaos1_waist_indices.inc"
};
static Uint16 event_chaos1_shoulder_plus_z_indices[10] = {
#include "shared/chaos1/event_chaos1_shoulder_plus_z_indices.inc"
};
static Uint16 event_chaos1_shoulder_minus_z_indices[10] = {
#include "shared/chaos1/event_chaos1_shoulder_minus_z_indices.inc"
};
static Uint16 event_chaos1_instep_plus_z_indices[10] = {
#include "shared/chaos1/event_chaos1_instep_plus_z_indices.inc"
};
static Uint16 event_chaos1_instep_minus_z_indices[10] = {
#include "shared/chaos1/event_chaos1_instep_minus_z_indices.inc"
};

static struct weld event_chaos1_welds[18] = {
    {&event_chaos1_object, &event_chaos1_upper_arm_plus_z,
     &event_chaos1_forearm_plus_z, 5, WELD_TO_A, 0, 0, NULL,
     event_chaos1_elbow_plus_z_indices},
    {&event_chaos1_object, &event_chaos1_forearm_plus_z,
     &event_chaos1_hand_plus_z, 5, WELD_TO_A, 0, 0, NULL,
     event_chaos1_wrist_plus_z_indices},
    {&event_chaos1_object, &event_chaos1_hand_plus_z,
     &event_chaos1_fingers_plus_z, 5, WELD_TO_A, 0, 0, NULL,
     event_chaos1_knuckles_plus_z_indices},
    {&event_chaos1_object, &event_chaos1_upper_arm_minus_z,
     &event_chaos1_forearm_minus_z, 5, WELD_TO_A, 0, 0, NULL,
     event_chaos1_elbow_minus_z_indices},
    {&event_chaos1_object, &event_chaos1_forearm_minus_z,
     &event_chaos1_hand_minus_z, 5, WELD_TO_A, 0, 0, NULL,
     event_chaos1_wrist_minus_z_indices},
    {&event_chaos1_object, &event_chaos1_hand_minus_z,
     &event_chaos1_fingers_minus_z, 5, WELD_TO_A, 0, 0, NULL,
     event_chaos1_knuckles_minus_z_indices},
    {&event_chaos1_object, &event_chaos1_thigh_plus_z,
     &event_chaos1_shin_plus_z, 5, WELD_TO_A, 0, 0, NULL,
     event_chaos1_knee_plus_z_indices},
    {&event_chaos1_object, &event_chaos1_shin_plus_z,
     &event_chaos1_ankle_plus_z, 5, WELD_TO_A, 0, 0, NULL,
     event_chaos1_ankle_plus_z_indices},
    {&event_chaos1_object, &event_chaos1_foot_plus_z, &event_chaos1_toe_plus_z,
     4, WELD_TO_A, 0, 0, NULL, event_chaos1_toes_plus_z_indices},
    {&event_chaos1_object, &event_chaos1_thigh_minus_z,
     &event_chaos1_shin_minus_z, 5, WELD_TO_A, 0, 0, NULL,
     event_chaos1_knee_minus_z_indices},
    {&event_chaos1_object, &event_chaos1_shin_minus_z,
     &event_chaos1_ankle_minus_z, 5, WELD_TO_A, 0, 0, NULL,
     event_chaos1_ankle_minus_z_indices},
    {&event_chaos1_object, &event_chaos1_foot_minus_z,
     &event_chaos1_toe_minus_z, 4, WELD_TO_A, 0, 0, NULL,
     event_chaos1_toes_minus_z_indices},
    {&event_chaos1_object, &event_chaos1_torso, &event_chaos1_hips, 6,
     WELD_TO_B, 0, 0, NULL, event_chaos1_waist_indices},
    {&event_chaos1_object, &event_chaos1_torso, &event_chaos1_upper_arm_plus_z,
     5, WELD_TO_B, 0, 0, NULL, event_chaos1_shoulder_plus_z_indices},
    {&event_chaos1_object, &event_chaos1_torso, &event_chaos1_upper_arm_minus_z,
     5, WELD_TO_B, 0, 0, NULL, event_chaos1_shoulder_minus_z_indices},
    {&event_chaos1_object, &event_chaos1_ankle_plus_z,
     &event_chaos1_foot_plus_z, 5, WELD_TO_A, 0, 0, NULL,
     event_chaos1_instep_plus_z_indices},
    {&event_chaos1_object, &event_chaos1_ankle_minus_z,
     &event_chaos1_foot_minus_z, 5, WELD_TO_A, 0, 0, NULL,
     event_chaos1_instep_minus_z_indices},
    {NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL},
};

static void event_chaos1_init(struct task *task);
static void event_chaos1_step(struct task *task);
static void event_chaos1_draw(struct task *task);
static void event_chaos1_destroy(struct task *task);

void event_chaos1_load_textures(void)
{
    njLoadTexture(&event_chaos1_texlist);
    njLoadTexture(&event_chaos0_water_texlist);
}

void event_chaos1_release_textures(void)
{
    njReleaseTexture(&event_chaos1_texlist);
    njReleaseTexture(&event_chaos0_water_texlist);
}

void event_chaos1_create(Float x, Float y, Float z, Angle ang_x, Angle ang_y,
                         Angle ang_z, int steps)
{
    struct chaos1_state *state;
    struct task_work *work;

    event_chaos1_task = task_create(TASK_WORK, 5, event_chaos1_init);
    state = heap_alloc(sizeof(struct chaos1_state));
    work = event_chaos1_task->work;
    VARS(work)->state = state;
    work->mode = CHAOS1_STAND;
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
    state->lift_speed = CHAOS1_DEPTH / state->form_steps;
    state->puddle_scale.x = 3.0F;
    state->puddle_scale.y = 2.0F;
    state->puddle_scale.z = 3.0F;
    state->form_speed.x = 0.2F / state->form_steps;
    state->form_speed.y = 1.0F / state->form_steps;
    state->form_speed.z = 0.2F / state->form_steps;
    state->form.x = 0.8F;
    state->form.y = 0.01F;
    state->form.z = 0.8F;
    state->alpha = 0.05F;
    state->alpha_speed = 0.5F / state->form_steps;
    event_weld(event_chaos1_welds, event_chaos1_action, VARS(work)->frame,
               CHAOS1_NODES, PLAYER_WELD_MAKE);
}

void event_chaos1_kill(void)
{
    if (event_chaos1_task != NULL) {
        task_kill(event_chaos1_task);
    }
}

void event_chaos1_melt(void)
{
    struct task_work *work = event_chaos1_task->work;

    VARS(work)->timer = 0;
    work->mode = CHAOS1_MELT;
}

void event_chaos1_rise(void)
{
    struct task_work *work = event_chaos1_task->work;

    VARS(work)->timer = 0;
    work->mode = CHAOS1_RISE;
}

void event_chaos1_set_speed(Float speed)
{
    struct task_work *work = event_chaos1_task->work;
    struct chaos1_state *state = VARS(work)->state;

    state->action_speed = speed;
}

void event_chaos1_set_pos(Float x, Float y, Float z)
{
    struct task_work *work = event_chaos1_task->work;

    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
}

void event_chaos1_set_ang(Angle x, Angle y, Angle z)
{
    struct task_work *work = event_chaos1_task->work;

    work->ang.x = x;
    work->ang.y = y;
    work->ang.z = z;
}

void event_chaos1_puddle(void)
{
    struct task_work *work = event_chaos1_task->work;
    struct chaos1_state *state = VARS(work)->state;

    state->lift = -CHAOS1_DEPTH;
    state->form.x = 1.0F;
    state->form.y = 1.0F;
    state->form.z = 1.0F;
    state->alpha = 0.6F;
    work->mode = CHAOS1_PUDDLE;
}

static void event_chaos1_init(struct task *task)
{
    task->step = event_chaos1_step;
    task->draw = event_chaos1_draw;
    task->destructor = event_chaos1_destroy;
}

static void event_chaos1_step(struct task *task)
{
    struct task_work *work = task->work;
    struct chaos1_state *state = VARS(work)->state;

    event_weld(event_chaos1_welds, event_chaos1_action, VARS(work)->frame,
               CHAOS1_NODES, PLAYER_WELDING);
    switch (work->mode) {
    case CHAOS1_STAND:
        VARS(work)->frame += state->action_speed;
        if (VARS(work)->frame >= event_chaos1_action->motion->nbFrame) {
            VARS(work)->frame = 0.0F;
        }
        break;
    case CHAOS1_MELT:
        VARS(work)->frame += state->action_speed;
        if (VARS(work)->frame >= event_chaos1_action->motion->nbFrame) {
            VARS(work)->frame = 0.0F;
        }
        if (++VARS(work)->timer > state->form_steps) {
            work->mode = CHAOS1_PUDDLE;
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
    case CHAOS1_RISE:
        VARS(work)->frame += state->action_speed;
        if (VARS(work)->frame >= event_chaos1_action->motion->nbFrame) {
            VARS(work)->frame = 0.0F;
        }
        if (++VARS(work)->timer > state->form_steps) {
            work->mode = CHAOS1_STAND;
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
    case CHAOS1_PUDDLE:
        ++VARS(work)->timer;
        break;
    }
    event_chaos1_draw(task);
}

static void event_chaos1_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct chaos1_state *state = VARS(work)->state;
    /* @hack spelling: CHAOS1_BODY_HEIGHT through the local height, assigned
     * ahead of the local draw. SHC then pools 10.0 ahead of draw_tree's
     * address, as the image does (0x0cb83c84 and 0x0cb83c88 in EV0008); without
     * it, or with height assigned after draw, the two trade places and 2 of 235
     * words differ */
    Float height;
    /* @hack spelling: draw_tree called through a local, which SHC keeps in R8
     * through the switch (MOV.L at 0x0cb83b82 in EV0008). called by name, the
     * draw runs 2 bytes short, and 184 of 235 words differ */
    void (*draw)(NJS_OBJECT *object);
    /* @hack redundant: 24 bytes of frame that no code reads, of a type unknown,
     * declared as angles and a point. the image's ADD #-28,R15 reserves them
     * (0x0cb83b1c in EV0008); without them, 2 of 235 words differ */
    struct angles unused_ang;
    NJS_POINT3 unused_pos;

    draw_set_palette(DRAW_PLAYER_PALETTE);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njTranslate(NULL, 0.0F, 2.0F, 0.0F);
    height = CHAOS1_BODY_HEIGHT;
    draw = draw_tree;
    switch (work->mode) {
    case CHAOS1_STAND:
        njSetTexture(&event_chaos1_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, state->lift + height, 0.0F);
        draw_action(event_chaos1_action, VARS(work)->frame);
        njPopMatrix(1);
        break;
    case CHAOS1_MELT:
        njSetTexture(&event_chaos1_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, state->lift + height, 0.0F);
        draw_action(event_chaos1_action, VARS(work)->frame);
        njPopMatrix(1);
        njSetTexture(&event_chaos0_water_texlist);
        njPushMatrix(NULL);
        njScaleV(NULL, &state->form);
        njScaleV(NULL, &state->puddle_scale);
        draw(&event_chaos0_puddle_object);
        njPopMatrix(1);
        break;
    case CHAOS1_RISE:
        njSetTexture(&event_chaos1_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, state->lift + height, 0.0F);
        draw_action(event_chaos1_action, VARS(work)->frame);
        njPopMatrix(1);
        /* the rise draws the puddle with his texlist still set, where the
         * melt sets event_chaos0_water_texlist */
        njPushMatrix(NULL);
        njScaleV(NULL, &state->form);
        njScaleV(NULL, &state->puddle_scale);
        draw(&event_chaos0_puddle_object);
        njPopMatrix(1);
        break;
    case CHAOS1_PUDDLE:
        njSetTexture(&event_chaos0_water_texlist);
        njPushMatrix(NULL);
        njScaleV(NULL, &state->puddle_scale);
        njScale(NULL,
                1.0F + (njSin((Angle)(VARS(work)->timer * ANGLE_PER_DEGREE)) *
                        CHAOS1_SWAY),
                1.0F,
                1.0F + (njCos((Angle)(VARS(work)->timer * ANGLE_PER_DEGREE)) *
                        CHAOS1_SWAY));
        draw(&event_chaos0_puddle_object);
        njPopMatrix(1);
        break;
    }
    njPopMatrix(1);
    draw_set_palette(0);
}

static void event_chaos1_destroy(struct task *task)
{
    struct task_work *work = task->work;

    event_weld(event_chaos1_welds, event_chaos1_action, VARS(work)->frame,
               CHAOS1_NODES, PLAYER_WELD_FREE);
    if (VARS(work)->state != NULL) {
        heap_free(VARS(work)->state);
    }
}

/* an event's Egg Mobile: Eggman in his craft. it bobs, flies and turns as
 * the event asks. it plays its own action while no motion is queued */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/ground.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/motion.h"
#include "effect/dust.h"
#include "effect/glare.h"
#include "effect/panlight.h"
#include "event/event.h"
#include "event/play.h"
#include "player/player.h"
#include "shared/eggmobile.h"
#include "shared/eggmobile_action.h"
#include "shared/motion.h"
#include "system/task.h"
#include "system/util.h"

/* each event that links eggmobile.c defines these. the nodes of Eggman's
 * arms are those the welds join */
extern NJS_TEXLIST event_eggmobile_texlist;
extern NJS_OBJECT event_eggmobile_object;
extern NJS_OBJECT event_eggmobile_upper_arm_plus_z;
extern NJS_OBJECT event_eggmobile_forearm_plus_z;
extern NJS_OBJECT event_eggmobile_hand_plus_z;
extern NJS_OBJECT event_eggmobile_fingers_plus_z;
extern NJS_OBJECT event_eggmobile_upper_arm_minus_z;
extern NJS_OBJECT event_eggmobile_forearm_minus_z;
extern NJS_OBJECT event_eggmobile_hand_minus_z;
extern NJS_OBJECT event_eggmobile_fingers_minus_z;

#define EGGMOBILE_BOB_HEIGHT 2.0F /* the bob's height until set_bob */
#define EGGMOBILE_BOB_SPEED 0x100 /* the bob's angle a step until set_bob */
#define EGGMOBILE_PUFF_STEPS 5 /* the steps from one puff to the next */
#define EGGMOBILE_LAST_PUFF 5 /* the last puff's number, from 0: six in all */

/* the craft's move record, which its first step allocates and its
 * destructor frees */
struct eggmobile_move {
    NJS_VECTOR speed; /* added to its place each step of a move */
    struct angles turn; /* added to its angles each step of a turn */
    NJS_POINT3 to; /* where it's put as the move ends */
    Sint32 steps; /* the steps a move or a turn takes */
    Sint32 timer; /* the steps it has taken */
    Float bob; /* how far above its place the craft draws */
    Float bob_height; /* the most bob reaches */
    Angle bob_phase; /* bob is bob_height times its sine */
    Angle bob_speed; /* added to bob_phase each step */
};

/* work->vars holds the move record at p[0]. EVENT_MOTION keeps the event
 * motion, which plays the craft's action, at p[2] */
struct eggmobile_vars {
    struct eggmobile_move *move;
};

#define VARS(work) ((struct eggmobile_vars *)&(work)->vars)

/* a jet, whose glow panlight_create makes each step the jet is on. from and
 * to are points in the craft's frame. the glow starts at from, and its speed
 * is to less from */
struct eggmobile_jet {
    struct dust_params *params; /* the glow's, but for pos and speed */
    NJS_POINT3 from;
    NJS_POINT3 to;
};

/* the nozzle's puffs, which dust_create_from makes. event_eggmobile_puff
 * sets pos and speed for each */
static struct dust_params event_eggmobile_puff_params = {
    1.3F,
    0.15F,
    0.1F,
    0.05F,
    0.005F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {255.0F, 0.28F, 0.0F, 0.9F},
};

/* the three jets' glows */
static struct dust_params event_eggmobile_jet_params[3] = {
#include "shared/eggmobile/event_eggmobile_jet_params.inc"
};

static struct eggmobile_jet event_eggmobile_jets[3] = {
    {&event_eggmobile_jet_params[0],
     {7.747F, 4.886F, 8.1F},
     {9.747F, 4.886F, 8.1F}},
    {&event_eggmobile_jet_params[1],
     {13.371F, 10.077F, 0.0F},
     {15.371F, 10.077F, 0.0F}},
    {&event_eggmobile_jet_params[2],
     {7.747F, 4.886F, -8.1F},
     {9.747F, 4.886F, -8.1F}},
};

/* the nozzle at the craft's back, where the puffs and the glare start. the
 * glare shines toward event_eggmobile_glare_aim, a unit behind it */
static NJS_POINT3 event_eggmobile_nozzle = {-9.714F, -0.6F, 0.0F};
static NJS_POINT3 event_eggmobile_glare_aim = {-10.714F, -0.6F, 0.0F};

static struct motion_entry event_eggmobile_actions[2] = {
    {&event_eggmobile_action, 24, MOTION_LOOP, 0, 1.0F, 1.0F},
    {NULL, 0, 0, 0, 0.0F, 0.0F},
};

/* each weld's vertex pairs: a vertex's index in object_a's model, then in
 * object_b's. both elbows' welds share one table, and both hands' the
 * other */
static Uint16 event_eggmobile_elbow_indices[10] = {
#include "shared/eggmobile/event_eggmobile_elbow_indices.inc"
};
static Uint16 event_eggmobile_finger_indices[8] = {
#include "shared/eggmobile/event_eggmobile_finger_indices.inc"
};

static struct weld event_eggmobile_welds[5] = {
    {&event_eggmobile_object, &event_eggmobile_upper_arm_plus_z,
     &event_eggmobile_forearm_plus_z, 5, WELD_TO_B, 0, 0, NULL,
     event_eggmobile_elbow_indices},
    {&event_eggmobile_object, &event_eggmobile_upper_arm_minus_z,
     &event_eggmobile_forearm_minus_z, 5, WELD_TO_B, 0, 0, NULL,
     event_eggmobile_elbow_indices},
    {&event_eggmobile_object, &event_eggmobile_hand_plus_z,
     &event_eggmobile_fingers_plus_z, 4, WELD_TO_B, 0, 0, NULL,
     event_eggmobile_finger_indices},
    {&event_eggmobile_object, &event_eggmobile_hand_minus_z,
     &event_eggmobile_fingers_minus_z, 4, WELD_TO_B, 0, 0, NULL,
     event_eggmobile_finger_indices},
    {NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL},
};

/* the last craft's task, or null before the first create and after
 * event_eggmobile_kill */
static struct task *event_eggmobile_task = NULL;

/* @hack spelling: four helpers the pragma expands wherever this file calls
 * them, as event_eggmobile_jet's vector_diff is its FSUBs at 0x0cb82634,
 * 0x0cb8263e and 0x0cb82648 in EV0007. without the pragma, each call is a
 * BSR to the helper's own copy, and summed over the four functions that call
 * them, 519 of 703 words differ */
#pragma inline(vector_add, vector_diff, vector_copy, eggmobile_bob)
static void vector_add(Float *out, const Float *from, const Float *by)
{
    *out++ = *from++ + *by++;
    *out++ = *from++ + *by++;
    *out = *from + *by;
}

static void vector_diff(const Float *from, const Float *to, Float *out)
{
    *out++ = *to++ - *from++;
    *out++ = *to++ - *from++;
    *out = *to - *from;
}

static void eggmobile_bob(struct eggmobile_move *move)
{
    move->bob = move->bob_height * njSin(move->bob_phase);
    move->bob_phase = (Uint16)(move->bob_phase + move->bob_speed);
}

static void vector_copy(const Float *from, Float *to)
{
    *to++ = *from++;
    *to++ = *from++;
    *to = *from;
}

static void event_eggmobile_destroy(struct task *task)
{
    if (VARS(task->work)->move != NULL) {
        heap_free(VARS(task->work)->move);
    }
    event_motion_free(task->work);
}

static void event_eggmobile_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct eggmobile_move *move = VARS(work)->move;
    struct event_motion_control *motion;
    int index;
    Angle ang;

    /* @hack spelling: the motion read into task, then cast to motion. SHC then
     * loads it into task's R4 (MOV.L @(16,R14),R4 at 0x0cb823a0 in EV0007) and
     * copies it to R13; without it, 98 of 106 words differ */
    task = EVENT_MOTION(work);
    motion = (struct event_motion_control *)task;
    index = motion->action;
    if (motion->state == EVENT_MOTION_BLENDING) {
        index = motion->current;
    }
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + move->bob, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    if (work->event->motions != NULL) {
        event_actor_draw(work);
    } else {
        njSetTexture(&event_eggmobile_texlist);
        draw_set_palette(DRAW_PLAYER_PALETTE);
        if (motion->state != EVENT_MOTION_BLENDING) {
            draw_action(motion->actions[index].action, motion->frame);
        } else {
            draw_action(motion->blend, motion->frame);
        }
        draw_set_palette(0);
    }
    njPopMatrix(1);
}

static void event_eggmobile_puff(struct task *task)
{
    struct task_work *work = task->work;
    struct eggmobile_move *move = VARS(work)->move;
    Angle ang;
    /* @hack redundant: 12 bytes of frame that no code reads, of a type unknown,
     * declared as a vector. the image's ADD #-28,R15 reserves them (0x0cb8246a
     * in EV0007); without them, 2 of 120 words differ */
    NJS_VECTOR unused;
    NJS_POINT3 point;

    if (work->tick < EGGMOBILE_PUFF_STEPS) {
        return;
    }
    if (TASK_TIMER(work) > EGGMOBILE_LAST_PUFF) {
        work->submode = EGGMOBILE_JETS_OFF;
        work->tick = 0;
        work->timer = 0;
        return;
    }
    work->tick = 0;
    njPushMatrix(&_unit_matrix_);
    njTranslate(NULL, work->pos.x, work->pos.y + move->bob, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njCalcPoint(NULL, &event_eggmobile_nozzle, &point);
    njPopMatrix(1);
    vector_copy(&point.x, &event_eggmobile_puff_params.pos.x);
    vector_diff(&work->pos.x, &point.x, &event_eggmobile_puff_params.speed.x);
    njUnitVector(&event_eggmobile_puff_params.speed);
    dust_create_from(&event_eggmobile_puff_params);
    work->timer++;
}

static void event_eggmobile_jet(struct task *task, struct eggmobile_jet *jet)
{
    struct task_work *work = task->work;
    struct eggmobile_move *move = VARS(work)->move;
    Angle ang;
    NJS_POINT3 to;

    njPushMatrix(&_unit_matrix_);
    njTranslate(NULL, work->pos.x, work->pos.y + move->bob, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njCalcPoint(NULL, &jet->from, &jet->params->pos);
    njCalcPoint(NULL, &jet->to, &to);
    njPopMatrix(1);
    vector_diff(&jet->params->pos.x, &to.x, &jet->params->speed.x);
    panlight_create(jet->params);
}

static void event_eggmobile_glare(struct task *task)
{
    struct task_work *work = task->work;
    struct eggmobile_move *move = VARS(work)->move;
    Angle ang;
    NJS_POINT3 pos;
    NJS_VECTOR dir;
    NJS_ARGB color;
    Float size;

    njPushMatrix(&_unit_matrix_);
    njTranslate(NULL, work->pos.x, work->pos.y + move->bob, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njCalcPoint(NULL, &event_eggmobile_nozzle, &pos);
    njCalcPoint(NULL, &event_eggmobile_glare_aim, &dir);
    njPopMatrix(1);
    vector_diff(&pos.x, &dir.x, &dir.x);
    njUnitVector(&dir);
    size = 0.9F;
    color.a = 0.9F;
    color.r = 1.0F;
    color.g = 1.0F;
    color.b = 0.95F;
    glare_draw(&pos, &dir, size, &color);
}

#pragma inline(vector_add_split)
/* @hack spelling: vector_add with from's steps in statements of their own.
 * event_eggmobile_step's slide then keeps the place in R4 and the speed in R5
 * (0x0cb82826 to 0x0cb8282a in EV0007), as the image does. as a vector_add
 * call, the two trade places and 9 of event_eggmobile_step's 378 words
 * differ */
static void vector_add_split(Float *out, const Float *from, const Float *by)
{
    *out++ = *from + *by++;
    from++;
    *out++ = *from + *by++;
    from++;
    *out = *from + *by;
}

static void event_eggmobile_step(struct task *task)
{
    struct task_work *work = task->work;
    struct event_motion_control *motion = EVENT_MOTION(work);
    struct event_work *event = work->event;
    struct eggmobile_move *move = VARS(work)->move;
    NJS_POINT3 shadow;
    struct angles slope;
    /* @hack type: current, volatile, a copy of the motion on the stack
     * (MOV.L R9,@R15 at 0x0cb8275c in EV0007) that event_motion_step reads;
     * without it, the step runs 4 bytes short and 218 of 378 words differ */
    struct event_motion_control *volatile current = motion;
    Float x;
    Float y;
    Float z;

    switch (work->mode) {
    case EGGMOBILE_HOVER:
        eggmobile_bob(move);
        break;
    case EGGMOBILE_FLY:
        vector_add(&work->pos.x, &work->pos.x, &move->speed.x);
        eggmobile_bob(move);
        if (++move->timer >= move->steps) {
            vector_copy(&move->to.x, &work->pos.x);
            eggmobile_bob(move);
            work->mode = EGGMOBILE_HOVER;
        }
        break;
    case EGGMOBILE_SLIDE:
        vector_add_split(&work->pos.x, &work->pos.x, &move->speed.x);
        if (++move->timer >= move->steps) {
            vector_copy(&move->to.x, &work->pos.x);
            work->mode = EGGMOBILE_HOVER;
        }
        break;
    case EGGMOBILE_TURN:
        work->ang.x += move->turn.x;
        work->ang.y += move->turn.y;
        work->ang.z += move->turn.z;
        eggmobile_bob(move);
        if (++move->timer >= move->steps) {
            work->mode = EGGMOBILE_HOVER;
            work->submode = EGGMOBILE_JETS_OFF;
        }
        break;
    case EGGMOBILE_FLY_TURN:
        vector_add(&work->pos.x, &work->pos.x, &move->speed.x);
        work->ang.x += move->turn.x;
        work->ang.y += move->turn.y;
        work->ang.z += move->turn.z;
        eggmobile_bob(move);
        if (++move->timer >= move->steps) {
            vector_copy(&move->to.x, &work->pos.x);
            work->mode = EGGMOBILE_HOVER;
            work->submode = EGGMOBILE_JETS_OFF;
        }
        break;
    case EGGMOBILE_STILL:
        break;
    }
    switch (work->submode) {
    case EGGMOBILE_JETS_OFF:
        break;
    case EGGMOBILE_JETS_ALL:
        event_eggmobile_jet(task, &event_eggmobile_jets[0]);
        event_eggmobile_jet(task, &event_eggmobile_jets[1]);
        event_eggmobile_jet(task, &event_eggmobile_jets[2]);
        break;
    case EGGMOBILE_JETS_MINUS_Z:
        event_eggmobile_jet(task, &event_eggmobile_jets[2]);
        break;
    case EGGMOBILE_JETS_PLUS_Z:
        event_eggmobile_jet(task, &event_eggmobile_jets[0]);
        break;
    case EGGMOBILE_JETS_GLARE:
        event_eggmobile_glare(task);
        break;
    case EGGMOBILE_JETS_PUFF:
        event_eggmobile_puff(task);
        work->tick++;
        break;
    }
    x = work->pos.x;
    y = work->pos.y;
    z = work->pos.z;
    /* @bug mismatched-call: the step reads ground_below's height from R0 as an
     * int (LDS R0,FPUL and FLOAT at 0x0cb829ea and 0x0cb829ee in EV0007),
     * though ground_below returns it in FR0. the craft's height under
     * EVENT_GROUND, and its shadow's less SHADOW_LIFT, are whatever R0 holds */
#ifdef GCC
    /* repairs the height the tag names: GCC reads the Float ground_below
     * returns */
    shadow.y = ground_below(x, y, z, &slope);
#else
    shadow.y = ground_below_int(x, y, z, &slope);
#endif
    if (event->flags & EVENT_GROUND) {
        work->pos.y = shadow.y;
    }
    if (event->flags & EVENT_SLOPE) {
        work->ang.x = slope.x;
        work->ang.z = slope.z;
    }
    if (event->shadow > 0.0F) {
        shadow.x = x;
        shadow.y += SHADOW_LIFT;
        shadow.z = z;
        /* @hack spelling: work->event read again for the shadow's size (MOV.L
         * @(60,R13),R3 at 0x0cb82a3a in EV0007), though R10 holds it as event;
         * without it, the step runs 4 bytes short and 37 of 378 words differ */
        shadow_draw_at(&slope, &shadow, work->event->shadow);
    }
    event_motion_step(current);
    event_motion_weld(work, motion);
    event_eggmobile_draw(task);
}

static void event_eggmobile_init(struct task *task)
{
    struct task_work *work = task->work;
    struct eggmobile_move *move;
    struct event_motion_control *motion;

    work->mode = EGGMOBILE_HOVER;
    work->submode = EGGMOBILE_JETS_OFF;
    work->tick = 0;
    work->timer = 0;
    move = heap_alloc(sizeof(*move));
    move->bob = 0.0F;
    move->bob_height = EGGMOBILE_BOB_HEIGHT;
    move->bob_speed = EGGMOBILE_BOB_SPEED;
    move->bob_phase = 0;
    VARS(work)->move = move;
    event_motion_alloc(work);
    motion = EVENT_MOTION(work);
    motion->actions = event_eggmobile_actions;
    motion->welds = event_eggmobile_welds;
    motion->action = 0;
    event_alloc(work);
    event_set_flags(task, EVENT_IGNORE_GROUND);
    task->step = event_eggmobile_step;
    task->draw = event_eggmobile_draw;
    task->destructor = event_eggmobile_destroy;
}

void event_eggmobile_create(Float x, Float y, Float z, Angle ang_x, Angle ang_y,
                            Angle ang_z)
{
    struct task *task = task_create(TASK_WORK, 5, event_eggmobile_init);
    struct task_work *work;

    if (task != NULL) {
        work = task->work;
        work->pos.x = x;
        work->pos.y = y;
        work->pos.z = z;
        work->ang.x = ang_x;
        work->ang.y = ang_y;
        work->ang.z = ang_z;
        work->scl.x = 1.0F;
        work->scl.y = 1.0F;
        work->scl.z = 1.0F;
        event_eggmobile_task = task;
    }
}

void event_eggmobile_kill(void)
{
    if (event_eggmobile_task != NULL) {
        task_kill(event_eggmobile_task);
        event_eggmobile_task = NULL;
    }
}

void event_eggmobile_set_mode(enum eggmobile_mode mode)
{
    if (event_eggmobile_task != NULL) {
        event_eggmobile_task->work->mode = mode;
    }
}

void event_eggmobile_set_jets(Sint8 jets)
{
    if (event_eggmobile_task != NULL) {
        event_eggmobile_task->work->submode = jets;
    }
}

#pragma inline(eggmobile_set_move)
/* @hack spelling: steps and the move as the first two parameters, which the
 * image keeps in R4 and R5 (MOV.L @(8,R14),R5 at 0x0cb82be2 in EV0007). with
 * steps last, as the callers take it, event_eggmobile_fly_to and
 * event_eggmobile_slide_to run 4 bytes long together, and 52 of their 74 words
 * differ */
static void eggmobile_set_move(int steps, struct eggmobile_move *move,
                               struct task_work *work, Float x, Float y,
                               Float z)
{
    move->to.x = x;
    move->to.y = y;
    move->to.z = z;
    move->speed.x = (x - work->pos.x) / (Float)steps;
    move->speed.y = (y - work->pos.y) / (Float)steps;
    move->speed.z = (z - work->pos.z) / (Float)steps;
    move->steps = steps;
    move->timer = 0;
}

void event_eggmobile_fly_to(Float x, Float y, Float z, int steps)
{
    struct task_work *work = event_eggmobile_task->work;

    eggmobile_set_move(steps, VARS(work)->move, work, x, y, z);
    work->mode = EGGMOBILE_FLY;
}

void event_eggmobile_slide_to(Float x, Float y, Float z, int steps)
{
    struct task_work *work = event_eggmobile_task->work;

    eggmobile_set_move(steps, VARS(work)->move, work, x, y, z);
    work->mode = EGGMOBILE_SLIDE;
}

void event_eggmobile_turn(Angle x, Angle y, Angle z, int steps, Sint8 quiet)
{
    struct task_work *work = event_eggmobile_task->work;
    struct eggmobile_move *move = VARS(work)->move;

    move->turn.x = x / steps;
    move->turn.y = y / steps;
    move->turn.z = z / steps;
    move->steps = steps;
    move->timer = 0;
    work->mode = EGGMOBILE_TURN;
    if (!quiet) {
        if (y > 0) {
            work->submode = EGGMOBILE_JETS_MINUS_Z;
        }
        if (y < 0) {
            work->submode = EGGMOBILE_JETS_PLUS_Z;
        }
    }
}

void event_eggmobile_fly_turn(Float x, Float y, Float z, Angle ang_x,
                              Angle ang_y, Angle ang_z, int steps, Sint8 quiet)
{
    struct task_work *work = event_eggmobile_task->work;
    struct eggmobile_move *move = VARS(work)->move;

    move->to.x = x;
    move->to.y = y;
    move->to.z = z;
    move->speed.x = (x - work->pos.x) / (Float)steps;
    move->speed.y = (y - work->pos.y) / (Float)steps;
    move->speed.z = (z - work->pos.z) / (Float)steps;
    move->turn.x = ang_x / steps;
    move->turn.y = ang_y / steps;
    move->turn.z = ang_z / steps;
    move->steps = steps;
    move->timer = 0;
    work->mode = EGGMOBILE_FLY_TURN;
    if (!quiet) {
        if (ang_y > 0) {
            work->submode = EGGMOBILE_JETS_MINUS_Z;
        }
        if (ang_y < 0) {
            work->submode = EGGMOBILE_JETS_PLUS_Z;
        }
    }
}

void event_eggmobile_set_bob(Angle speed, Float height)
{
    struct eggmobile_move *move = VARS(event_eggmobile_task->work)->move;

    if (height >= 0.0F) {
        move->bob_height = height;
    }
    if (speed >= 0) {
        move->bob_speed = speed;
    }
}

struct task *event_eggmobile_get_task(void)
{
    return event_eggmobile_task;
}

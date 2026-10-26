/* EV0034.PRS's scorched Egg Mobile: Eggman in his craft, with a scorched
 * face. the craft loops its one action and bobs where it stands. its welds
 * close Eggman's elbows and fingers, as shared/eggmobile.c's do */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "draw/model.h"
#include "draw/motion.h"
#include "ev0034/scorched.h"
#include "ev0034/scorched_textures.h"
#include "event/event.h"
#include "player/player.h"
#include "shared/motion.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_ACTION ev0034_scorched_action;
extern NJS_OBJECT ev0034_scorched_object;
extern NJS_OBJECT ev0034_scorched_upper_arm_plus_z;
extern NJS_OBJECT ev0034_scorched_forearm_plus_z;
extern NJS_OBJECT ev0034_scorched_hand_plus_z;
extern NJS_OBJECT ev0034_scorched_fingers_plus_z;
extern NJS_OBJECT ev0034_scorched_upper_arm_minus_z;
extern NJS_OBJECT ev0034_scorched_forearm_minus_z;
extern NJS_OBJECT ev0034_scorched_hand_minus_z;
extern NJS_OBJECT ev0034_scorched_fingers_minus_z;

#define BOB_SPEED 0x200 /* the angle the bob's phase turns a step */
#define BOB_HEIGHT 1.5F /* the most the bob lifts or lowers the craft */

/* the craft's view of work->vars. its event motion takes the last var, as
 * EVENT_MOTION reads it */
struct scorched_vars {
    Float bob; /* how far above work->pos the craft is drawn */
    Angle bob_phase;
};

#define VARS(work) ((struct scorched_vars *)&(work)->vars)

static struct motion_entry scorched_actions[2] = {
    {&ev0034_scorched_action, 24, MOTION_LOOP, 0, 1.0F, 1.0F},
    {NULL, 0, 0, 0, 0.0F, 0.0F},
};

/* the elbow welds' indices: the 5 vertices each upper arm shares with its
 * forearm */
static Uint16 scorched_elbow_vertices[10] = {
#include "ev0034/scorched/scorched_elbow_vertices.inc"
};
/* the finger welds' indices: the 4 vertices each hand shares with its
 * fingers */
static Uint16 scorched_finger_vertices[8] = {
#include "ev0034/scorched/scorched_finger_vertices.inc"
};

static struct weld scorched_welds[5] = {
    {&ev0034_scorched_object, &ev0034_scorched_upper_arm_plus_z,
     &ev0034_scorched_forearm_plus_z, 5, WELD_TO_B, 0, 0, NULL,
     scorched_elbow_vertices},
    {&ev0034_scorched_object, &ev0034_scorched_upper_arm_minus_z,
     &ev0034_scorched_forearm_minus_z, 5, WELD_TO_B, 0, 0, NULL,
     scorched_elbow_vertices},
    {&ev0034_scorched_object, &ev0034_scorched_hand_plus_z,
     &ev0034_scorched_fingers_plus_z, 4, WELD_TO_B, 0, 0, NULL,
     scorched_finger_vertices},
    {&ev0034_scorched_object, &ev0034_scorched_hand_minus_z,
     &ev0034_scorched_fingers_minus_z, 4, WELD_TO_B, 0, 0, NULL,
     scorched_finger_vertices},
    {NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL},
};

static void scorched_destroy(struct task *task)
{
    event_motion_free(task->work);
}

static void scorched_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct event_motion_control *motion;
    int index;
    Angle ang;

    /* @hack spelling: the motion through task, a struct task *. SHC loads it
     * into task's R4 and copies it to R13 (MOV.L @(16,R14),R4 and MOV R4,R13
     * at 0x0cb84ef2). read straight into motion, scorched_draw runs 4 bytes
     * short, and 97 of its 103 words differ */
    task = EVENT_MOTION(work);
    motion = (struct event_motion_control *)task;
    index = motion->action;
    if (motion->state == EVENT_MOTION_BLENDING) {
        index = motion->current;
    }
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + VARS(work)->bob, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    if (work->event->motions != NULL) {
        event_actor_draw(work);
    } else {
        draw_set_palette(DRAW_PLAYER_PALETTE);
        njSetTexture(&ev0034_scorched_texlist);
        if (motion->state != EVENT_MOTION_BLENDING) {
            draw_action(motion->actions[index].action, motion->frame);
        } else {
            draw_action(motion->blend, motion->frame);
        }
        draw_set_palette(0);
    }
    njPopMatrix(1);
}

static void scorched_step(struct task *task)
{
    struct task_work *work = task->work;
    struct event_motion_control *motion = EVENT_MOTION(work);

    event_motion_step(motion);
    event_motion_weld(work, motion);
    VARS(work)->bob_phase += BOB_SPEED;
    VARS(work)->bob = njSin(VARS(work)->bob_phase) * BOB_HEIGHT;
    scorched_draw(task);
}

static void scorched_init(struct task *task)
{
    struct task_work *work = task->work;
    struct event_motion_control *motion;

    VARS(work)->bob = 0.0F;
    VARS(work)->bob_phase = 0;
    event_motion_alloc(work);
    motion = EVENT_MOTION(work);
    motion->actions = scorched_actions;
    motion->welds = scorched_welds;
    motion->action = 0;
    task->step = scorched_step;
    task->draw = scorched_draw;
    task->destructor = scorched_destroy;
}

struct task *ev0034_scorched_create(void)
{
    struct task *task = task_create(TASK_WORK, 3, scorched_init);

    if (task != NULL) {
        event_alloc(task->work);
    }
    return task;
}

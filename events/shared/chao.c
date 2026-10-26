/* an event's Chao, which draws the model each event defines */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/ground.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "event/event.h"
#include "event/play.h"
#include "shared/chao.h"
#include "shared/chao_textures.h"
#include "system/task.h"
#include "system/util.h"

#define CHAO_LIFT 1.4F /* how far above the ground EVENT_GROUND stands it */

struct chao_vars {
    Sint32 eyes; /* the texture its eyes show, as event_chao_set_eyes takes */
};

#define VARS(work) ((struct chao_vars *)&(work)->vars)

/* each event that links chao.c defines these. the draw draws
 * event_chao_object while no motion is queued. every Chao of the event
 * shares the two eyes */
extern NJS_OBJECT event_chao_object;
extern NJS_OBJECT event_chao_body;
extern NJS_OBJECT event_chao_eye_minus_x;
extern NJS_OBJECT event_chao_eye_plus_x;

static void event_chao_show_eyes(struct task_work *work, int texture);
static void event_chao_turn_body(NJS_OBJECT *object);

static void event_chao_destroy(struct task *task)
{
    event_chao_show_eyes(task->work, CHAO_EYES_PLAIN);
}

static void event_chao_draw(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: 24 bytes of frame that no code reads, of a type unknown,
     * declared as under and slope. the image's ADD #-28,R15 reserves them
     * (0x0cb80d06 in EV0022); without them, 2 of 108 words differ */
    NJS_POINT3 under;
    struct angles slope;
    Angle ang;

    njSetMotionCallback(event_chao_turn_body);
    if (work->event->motions != NULL) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        event_motions_draw(work);
        njPopMatrix(1);
    } else {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njSetTexture(&event_chao_texlist);
        draw_set_palette(DRAW_PLAYER_PALETTE);
        draw_tree(&event_chao_object);
        draw_set_palette(0);
        njPopMatrix(1);
    }
    njSetMotionCallback(NULL);
}

static void event_chao_step(struct task *task)
{
    struct task_work *work = task->work;
    struct event_work *event = work->event;
    NJS_POINT3 under;
    struct angles slope;
    Float x;
    Float y;
    Float z;

    if (event->paths != NULL) {
        event_paths_step(work);
    }
    x = work->pos.x;
    y = work->pos.y;
    z = work->pos.z;
    under.y = ground_below(x, y, z, &slope);
    if (event->flags & EVENT_GROUND) {
        work->pos.y = under.y + CHAO_LIFT;
    }
    if (event->flags & EVENT_SLOPE) {
        work->ang.x = slope.x;
        work->ang.z = slope.z;
    }
    if (event->shadow > 0.0F) {
        under.x = x;
        under.y += SHADOW_LIFT;
        under.z = z;
        /* @hack spelling: work->event read again for the shadow's size (MOV.L
         * @(60,R14),R3 at 0x0cb80e66 in EV0022), though R13 holds it as event;
         * without it, the step runs 2 bytes short and 29 of 87 words differ */
        shadow_draw_at(&slope, &under, work->event->shadow);
    }
    event_chao_show_eyes(work, VARS(work)->eyes);
    event_chao_draw(task);
}

static void event_chao_init(struct task *task)
{
    VARS(task->work)->eyes = CHAO_EYES_PLAIN;
    task->step = event_chao_step;
    task->draw = event_chao_draw;
    task->destructor = event_chao_destroy;
}

struct task *event_chao_create(void)
{
    struct task *task;

    task = task_create(TASK_WORK, 3, event_chao_init);
    if (task != NULL) {
        event_alloc(task->work);
    }
    return task;
}

void event_chao_set_eyes(struct task *task, int texture)
{
    if (task != NULL) {
        VARS(task->work)->eyes = texture;
    }
}

/* the motion callback the draw sets. as a motion draws each node, it turns
 * event_chao_body by the camera's y angle, then its x angle */
static void event_chao_turn_body(NJS_OBJECT *object)
{
    Angle ang;

    if (object == &event_chao_body) {
        TURN_VIA(njRotateY, event_camera.ang.y, ang);
        TURN_VIA(njRotateX, event_camera.ang.x, ang);
    }
}

/* set texture on both eyes' material. every Chao of the event shares the eyes,
 * so each Chao's step sets its own before its draw */
static void event_chao_show_eyes(struct task_work *work, int texture)
{
    /* @hack spelling: each eye's model read through the eye's address, which
     * the image pools and reads 4 bytes past (MOV.L @(4,R3),R2 at 0x0cb80f02 in
     * EV0022); without it, 7 of 9 words differ */
    (&event_chao_eye_minus_x)->model->mats[0].attr_texId = texture;
    (&event_chao_eye_plus_x)->model->mats[0].attr_texId = texture;
}

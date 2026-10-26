/* the sky chase's events: act 1's, as its course nears its end, and act
 * 2's, half way. a scene plays a camera action and its own actions at one
 * frame, which moves on each step. act 1's event plays two scenes, and act
 * 2's one */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "draw/draw.h"
#include "shooting/act1_textures.h"
#include "shooting/act2_textures.h"
#include "shooting/event.h"
#include "shooting/event1_land_camera.h"
#include "system/task.h"

struct event_vars {
    Float frame; /* the frame the scene's camera and actions play at */
    Uint8 unk_04[4];
    Float timer; /* up 0.5 a step, wrapping at 30. nothing reads it */
};

#define VARS(work) ((struct event_vars *)&(work)->vars)

enum event_mode { EVENT_INIT, EVENT_FIRST, EVENT_SECOND, EVENT_STILL };

/* the textures both acts' scenes draw with, and the events' camera actions
 * and scene actions */
extern NJS_TEXLIST shooting0_texlist;
extern NJS_CACTION event1_camera;
extern NJS_CACTION event2_camera;
extern NJS_ACTION dat_0c9b932c;
extern NJS_ACTION carrier_propellers_action;
extern NJS_ACTION dat_0c994890;
extern NJS_ACTION dat_0c9b653c;
extern NJS_ACTION dat_0c9ddb30;
extern NJS_ACTION dat_0c9dee38;
extern NJS_ACTION carrier_action;
extern NJS_ACTION dat_0ca8584c;

/* an event has nothing of its own to free */
static void event_destroy(struct task *task)
{
}

static void event1_draw(struct task *task)
{
    struct task_work *work = task->work;

    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 0.0F, 0.0F);
    njSetTexture(&shooting0_texlist);
    draw_action(&dat_0c9b932c, VARS(work)->frame);
    draw_action(&carrier_propellers_action, VARS(work)->frame);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 0.0F, 0.0F);
    njSetTexture(&shooting1_texlist);
    draw_action(&dat_0c994890, VARS(work)->frame);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, -648.1F, -3698.0F);
    njSetTexture(&shooting0_texlist);
    draw_action(&dat_0c9b653c, VARS(work)->frame);
    njPopMatrix(1);
}

static void event1_land_draw(struct task *task)
{
    struct task_work *work = task->work;

    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 0.0F, 0.0F);
    njSetTexture(&shooting1_texlist);
    draw_action(&dat_0c9ddb30, VARS(work)->frame);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 0.0F, 0.0F);
    njSetTexture(&shooting1_texlist);
    draw_action(&dat_0c9dee38, VARS(work)->frame);
    njPopMatrix(1);
}

/* act 1's event: event1_camera's scene at a frame a step, then
 * event1_land_camera's at half a frame a step. each scene ends once the
 * frame reaches its camera motion's frame count */
void shooting_event1_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case EVENT_INIT:
        VARS(work)->frame = 0.0F;
        VARS(work)->timer = 0.0F;
        task->draw = event1_draw;
        task->destructor = event_destroy;
        work->mode = EVENT_FIRST;
        break;
    case EVENT_FIRST:
        camera_play_action(&event1_camera, VARS(work)->frame);
        event1_draw(task);
        VARS(work)->frame += 1.0F;
        /* @hack spelling: the motion read through event1_camera's address,
         * which the call's pool entry holds, at 4 past it (MOV.L @(4,R3),R2 at
         * 0x0c90089a); as event1_camera.motion, the field's address takes a
         * pool entry of its own, and 14 of 119 words differ */
        if (VARS(work)->frame >= (&event1_camera)->motion->nbFrame) {
            VARS(work)->frame = 0.0F;
            work->mode = EVENT_SECOND;
            task->draw = event1_land_draw;
        }
        VARS(work)->timer += 0.5F;
        if (VARS(work)->timer >= 30.0F) {
            VARS(work)->timer = 0.0F;
        }
        break;
    case EVENT_SECOND:
        camera_play_action(&event1_land_camera, VARS(work)->frame);
        event1_land_draw(task);
        VARS(work)->frame += 0.5F;
        /* @hack spelling: the same for event1_land_camera (0x0c9008fa); as
         * event1_land_camera.motion, 4 of 119 words differ */
        if (VARS(work)->frame >= (&event1_land_camera)->motion->nbFrame) {
            task_kill(task);
        }
        break;
    case EVENT_STILL:
        break;
    }
}

static void event2_draw(struct task *task)
{
    struct task_work *work = task->work;

    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 0.0F, 4001.0F);
    njSetTexture(&shooting0_texlist);
    njScale(NULL, 0.5F, 0.5F, 0.5F);
    draw_action(&carrier_action, 0.0F);
    draw_action(&carrier_propellers_action, VARS(work)->frame);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 0.0F, 0.0F);
    njSetTexture(&shooting2_texlist);
    draw_action(&dat_0ca8584c, VARS(work)->frame);
    njPopMatrix(1);
}

/* act 2's event: event2_camera's scene at a frame a step. it ends once the
 * frame reaches its camera motion's frame count */
void shooting_event2_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case EVENT_INIT:
        VARS(work)->frame = 0.0F;
        VARS(work)->timer = 0.0F;
        task->draw = event2_draw;
        task->destructor = event_destroy;
        work->mode = EVENT_FIRST;
        break;
    case EVENT_FIRST:
        camera_play_action(&event2_camera, VARS(work)->frame);
        event2_draw(task);
        VARS(work)->frame += 1.0F;
        /* @hack spelling: the motion read through event2_camera's address, as
         * in shooting_event1_step (MOV.L @(4,R3),R2 at 0x0c900a7a); as
         * event2_camera.motion, 7 of 77 words differ */
        if (VARS(work)->frame >= (&event2_camera)->motion->nbFrame) {
            task_kill(task);
        }
        VARS(work)->timer += 0.5F;
        if (VARS(work)->timer >= 30.0F) {
            VARS(work)->timer = 0.0F;
        }
        break;
    case EVENT_STILL:
        break;
    }
}

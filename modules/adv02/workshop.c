/* front of the workshop above the station, with a model of its own for night */
#include <ninja.h>

#include "adv02/texlists.h"
#include "adv02/workshop.h"
#include "camera/camera.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "progress/progress.h"
#include "system/task.h"

/* @hack type: both models' objects declared as arrays, and read through ->.
 * SHC then loads each object's address and reads its model 4 bytes past it, in
 * one MOV.L @(4,R4),R4 both draws share (0x0c90d8ce). as NJS_OBJECTs read
 * through ., 6 of workshop_draw's 37 words differ */
extern NJS_OBJECT workshop_object[];
extern NJS_OBJECT workshop_night_object[];

#define WORKSHOP_X 1432.5F
#define WORKSHOP_Y 239.0F
#define WORKSHOP_Z 780.0F
/* outside the set editor, the step draws the workshop only while the camera is
 * within DRAW_RANGE of it */
#define DRAW_RANGE 1000.0F

enum workshop_mode { WORKSHOP_INIT, WORKSHOP_RUN };

static void workshop_init(struct task *task);
static void workshop_place(struct task *task);
static void workshop_draw(struct task *task);
static void workshop_destroy(struct task *task);

static void workshop_update(struct task *task)
{
}

/* while the set editor holds the task, the step puts the workshop back at its
 * position each step, and draws it at any distance */
void workshop_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        switch (work->mode) {
        case WORKSHOP_INIT:
            workshop_init(task);
            break;
        case WORKSHOP_RUN:
            if (njDistanceP2P(&camera_work->pos, &work->pos) < DRAW_RANGE) {
                workshop_draw(task);
                workshop_update(task);
            }
            break;
        }
    } else {
        switch (work->mode) {
        case WORKSHOP_INIT:
            workshop_init(task);
            break;
        case WORKSHOP_RUN:
            workshop_place(task);
            workshop_draw(task);
            workshop_update(task);
            break;
        }
    }
}

static void workshop_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = workshop_draw;
    task->destructor = workshop_destroy;
    workshop_place(task);
    work->mode = WORKSHOP_RUN;
}

static void workshop_place(struct task *task)
{
    struct task_work *work = task->work;

    work->pos.x = WORKSHOP_X;
    work->pos.y = WORKSHOP_Y;
    work->pos.z = WORKSHOP_Z;
}

static void workshop_draw(struct task *task)
{
    struct task_work *work = task->work;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    if (story_get_time() == TIME_NIGHT) {
        njSetTexture(&mrobj_texlist);
        draw_model(workshop_night_object->model);
    } else {
        njSetTexture(&mrobj_texlist);
        draw_model(workshop_object->model);
    }
    njPopMatrix(1);
}

static void workshop_destroy(struct task *task)
{
}

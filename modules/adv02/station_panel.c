/* panel by the station, with a model of its own for night */
#include <ninja.h>

#include "adv02/station_panel.h"
#include "adv02/texlists.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "progress/progress.h"
#include "system/task.h"

/* @hack type: both models' objects declared as arrays, and read through ->.
 * SHC then loads each object's address and reads its model 4 bytes past it, in
 * one MOV.L @(4,R4),R4 both draws share (0x0c90da2e). as NJS_OBJECTs read
 * through ., 6 of station_panel_draw's 37 words differ */
extern NJS_OBJECT mystic_ruins_station_panel_object[];
extern NJS_OBJECT mystic_ruins_station_panel_night_object[];

#define PANEL_X (-45.0F)
#define PANEL_Y 90.0F
#define PANEL_Z 1180.0F

static void station_panel_init(struct task *task);
static void station_panel_place(struct task *task);
static void station_panel_draw(struct task *task);
static void station_panel_destroy(struct task *task);

enum station_panel_mode { PANEL_INIT, PANEL_RUN };

/* the panel tasks alive, which only the empty tests below read */
static int station_panel_count = 0;

static void station_panel_update(struct task *task)
{
}

/* while the set editor holds the task, the step puts the panel back at its
 * position each step */
void station_panel_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        switch (work->mode) {
        case PANEL_INIT:
            station_panel_init(task);
            break;
        case PANEL_RUN:
            station_panel_draw(task);
            station_panel_update(task);
            break;
        }
    } else {
        switch (work->mode) {
        case PANEL_INIT:
            station_panel_init(task);
            break;
        case PANEL_RUN:
            station_panel_place(task);
            station_panel_draw(task);
            station_panel_update(task);
            break;
        }
    }
}

static void station_panel_init(struct task *task)
{
    struct task_work *work = task->work;

    /* @hack redundant: an empty test of the count, the TST R3,R3 at 0x0c90d968;
     * without it, 14 of 21 words differ */
    if (station_panel_count == 0) {
    }
    station_panel_count++;
    task->draw = station_panel_draw;
    task->destructor = station_panel_destroy;
    station_panel_place(task);
    work->mode = PANEL_RUN;
}

static void station_panel_place(struct task *task)
{
    struct task_work *work = task->work;

    work->pos.x = PANEL_X;
    work->pos.y = PANEL_Y;
    work->pos.z = PANEL_Z;
}

static void station_panel_draw(struct task *task)
{
    struct task_work *work = task->work;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    if (story_get_time() == TIME_NIGHT) {
        njSetTexture(&mrobj_texlist);
        draw_model(mystic_ruins_station_panel_night_object->model);
    } else {
        njSetTexture(&mrobj_texlist);
        draw_model(mystic_ruins_station_panel_object->model);
    }
    njPopMatrix(1);
}

static void station_panel_destroy(struct task *task)
{
    station_panel_count--;
    /* @hack redundant: an empty test of the count, the TST R2,R2 at 0x0c90da46;
     * without it, 3 of 7 words differ */
    if (station_panel_count == 0) {
    }
}

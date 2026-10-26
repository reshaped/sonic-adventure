/* the race course's model, a fixture among the scenery's children */
#include <ninja.h>

#include "al_race/course_model.h"
#include "al_race/scenery.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"

/* the course's object */
extern NJS_OBJECT dat_0cbc65c0;

/* the fixture's object, in its task's motion */
#define COURSE_MODEL(task) ((NJS_OBJECT **)(task)->motion)

static void course_model_start(struct task *task, void *arg);
static void course_model_draw(struct task *task);

static const struct task_desc course_model_desc = {
    sizeof(NJS_OBJECT *), TASK_WORK,         course_model_start,
    course_model_draw,    course_model_draw, NULL};
static NJS_OBJECT *const course_model_object = &dat_0cbc65c0;

void course_model_create(void)
{
    scenery_add(&course_model_desc, (void *)&course_model_object);
}

static void course_model_start(struct task *task, void *arg)
{
    NJS_OBJECT *object = *(NJS_OBJECT **)arg;
    struct task_work *work = task->work;
    NJS_OBJECT **model = COURSE_MODEL(task);

    scenery_place(work, object);
    *model = object;
}

static void course_model_draw(struct task *task)
{
    NJS_OBJECT **model;

    if (frame_step == 0 && !placement_far_from_camera(task)) {
        model = COURSE_MODEL(task);
        texture_set_stage();
        draw_tree(*model);
    }
}

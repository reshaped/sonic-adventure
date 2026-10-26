/* the stretch of highway where the last story's last events play. its task
 * draws its one model each step */
#include <ninja.h>

#include "draw/tree.h"
#include "shared/highway.h"
#include "system/task.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 11
#define TEXT_3_LENGTH 11
#define TEXT_4_LENGTH 12
#define TEXT_5_LENGTH 12
#define TEXT_6_LENGTH 12
#define TEXT_7_LENGTH 12
#define TEXT_8_LENGTH 12
#define TEXT_9_LENGTH 12
#define TEXT_10_LENGTH 10
#define TEXT_11_LENGTH 10
#define TEXT_12_LENGTH 11
#include "shared/highway/text.inc"

/* the first of the highway's seven parts, which EV00FD, EV00FE and EV00FF
 * each define. each part's sibling is the next */
extern NJS_OBJECT event_highway_parts;

/* a hidden root without a model, whose child is the first part */
static NJS_OBJECT event_highway_object = {
#include "shared/highway/event_highway_object.inc"
};

static NJS_TEXNAME event_highway_texnames[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};

NJS_TEXLIST event_highway_texlist = {
#include "shared/highway/event_highway_texlist.inc"
};

/* the highway's task, or null before a create succeeds and after
 * event_highway_kill */
static struct task *event_highway = NULL;

static void event_highway_destroy(struct task *task)
{
}

static void event_highway_draw(struct task *task)
{
    njSetTexture(&event_highway_texlist);
    draw_tree(&event_highway_object);
}

static void event_highway_step(struct task *task)
{
    event_highway_draw(task);
}

static struct task *event_highway_spawn(void)
{
    struct task *task;

    task = task_create(0, 5, NULL);
    if (task != NULL) {
        task->step = event_highway_step;
        task->draw = event_highway_draw;
        task->destructor = event_highway_destroy;
    }
    /* @hack implicit: no return, so R0 still holds task_create's result
     * (MOV R0,R4 at 0x0cb860be in EV00FE), which event_highway_create stores
     * (0x0cb860e6); returned explicitly, the function runs 2 bytes long and 10
     * of 18 words differ */
} /* NOLINT(clang-diagnostic-return-type) */

void event_highway_create(void)
{
    if (event_highway == NULL) {
        event_highway = event_highway_spawn();
    }
}

void event_highway_kill(void)
{
    if (event_highway != NULL) {
        task_kill(event_highway);
        event_highway = NULL;
    }
}

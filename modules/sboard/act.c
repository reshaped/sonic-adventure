/* Sand Hill's act tasks, the children of a parent that lives as long as the
 * act */
#include <ninja.h>
#include <sg_xpt.h>

#include "effect/flare.h"
#include "sboard/act.h"
#include "sboard/gate_combo.h"
#include "sboard/rolling_rock.h"
#include "sboard/sand_worm.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/task_desc.h"

/* the parent's record, in task->motion: the stage and act it was made in */
struct act_record {
    Uint8 stage;
    Uint8 act;
};

static void act_parent_init(struct task *task, void *arg);
static void act_parent_step(struct task *task);

static const struct task_desc act_parent_desc = {
    sizeof(struct act_record), 0, act_parent_init, act_parent_step, NULL, NULL};
static const NJS_POINT3 sun_pos = {0.0F, 10000.0F, 0.0F};

/* the remembered parent: the last one made, or NULL before the first.
 * act_parent_kill and act_parent_destroy clear it. a parent that destroys
 * itself leaves it dangling */
static struct task *act_parent = NULL;

struct task *act_parent_create(void)
{
    /* @hack spelling: the result through task, which keeps the call a JSR
     * (0x0c905548); returned directly, it becomes a tail jump to
     * task_create_desc, and 9 of 9 words differ */
    struct task *task = task_create_desc(3, &act_parent_desc, NULL);

    return task;
}

void act_parent_kill(void)
{
    if (act_parent != NULL) {
        task_kill(act_parent);
        act_parent = NULL;
    }
}

void act_parent_destroy(void)
{
    if (act_parent != NULL) {
        task_destroy(act_parent);
        act_parent = NULL;
    }
}

struct task *act_task_create(const struct task_desc *desc, void *arg)
{
    struct task *parent = act_parent;
    struct task *task;

    task = parent == NULL ? NULL : task_create_desc_child(parent, desc, arg);
    return task;
}

static void act_parent_init(struct task *task, void *arg)
{
    struct act_record *record;
    Uint32 stage_act;

    act_parent = task;
    record = (struct act_record *)task->motion;
    stage_act = stage_get_stage_act();
    record->stage = STAGE_OF(stage_act);
    record->act = stage_act & 0xff;
    sand_worm_a_create();
    sand_worm_b_create();
    sand_worm_c_create();
    sand_worm_d_create();
    rolling_rock_a_create();
    rolling_rock_b_create();
    rolling_rock_c_create();
    gate_combo_create();
    act_task_create(&flare_desc, (void *)&sun_pos);
}

static void act_parent_step(struct task *task)
{
    Uint32 stage_act = stage_get_stage_act();
    struct act_record *record = (struct act_record *)task->motion;

    if (STAGE_OF(stage_act) != record->stage ||
        (stage_act & 0xff) != record->act) {
        task_destroy(task);
    } else {
        task_step_children(task);
    }
}

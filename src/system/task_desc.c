/* tasks created from descriptors, which give a kind of task's callbacks
 * and the size of its record */
#include <sg_xpt.h>

#include "system/task.h"
#include "system/task_desc.h"

struct task *task_create_desc(int level, const struct task_desc *desc,
                              void *arg)
{
    struct task *task;

    task = task_create(desc->create & ~TASK_MOTION, level, desc->step);
    if (task != NULL) {
        task = task_apply_desc(task, desc, arg);
    }
    return task;
}

struct task *task_create_desc_child(struct task *parent,
                                    const struct task_desc *desc, void *arg)
{
    struct task *task;

    task = task_create_child(desc->create & ~TASK_MOTION, desc->step, parent);
    if (task != NULL) {
        task = task_apply_desc(task, desc, arg);
    }
    return task;
}

struct task *task_apply_desc(struct task *task, const struct task_desc *desc,
                             void *arg)
{
    void (*init)(struct task *task, void *arg);
    void *record;
    /* @hack type: size, an int tested as a Uint32, which SHC loads once into
     * R4 for both the CMP/HI and the call (MOV.L @R13,R4 at 0x8c0a5940). as
     * a Uint32, size takes R12 and 39 of 43 words differ; read from desc in
     * place, the size loads twice and 9 of 43 words differ */
    int size;

    size = desc->size;
    if ((Uint32)size > 0) {
        record = heap_alloc(size);
        if (record == NULL) {
            task_kill(task);
            task = NULL;
        } else {
            task->motion = record;
        }
    }
    if (task != NULL) {
        task->step = desc->step;
        task->draw = desc->draw;
        task->destructor = desc->destructor;
        init = desc->init;
        if (init != NULL) {
            init(task, arg);
        }
    }
    return task;
}

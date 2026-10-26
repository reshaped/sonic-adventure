/* tasks created from descriptors, which give a kind of task's callbacks
 * and the size of its record */
#ifndef SYSTEM_TASK_DESC_H
#define SYSTEM_TASK_DESC_H
#include <sg_xpt.h>

#include "system/task.h"

/* a kind of task: its callbacks, and the size of a record task_apply_desc
 * allocates into task->motion. the record isn't cleared. task_destroy frees
 * it as a motion record */
struct task_desc {
    Uint32 size; /* the record's bytes, or 0 for none */
    /* task_create's flags. task_create_desc and task_create_desc_child drop
     * TASK_MOTION, since motion holds the record */
    int create;
    /* task_apply_desc's last call, with its arg, once the task has its
     * record and callbacks. it may be null */
    void (*init)(struct task *task, void *arg);
    void (*step)(struct task *task);
    void (*draw)(struct task *task);
    void (*destructor)(struct task *task);
};

/* create a task at level, or as parent's first child, and apply desc to it
 * with arg. returns null when the task can't be created, or when
 * task_apply_desc returns null */
struct task *task_create_desc(int level, const struct task_desc *desc,
                              void *arg);
struct task *task_create_desc_child(struct task *parent,
                                    const struct task_desc *desc, void *arg);
/* allocate desc's record, if size isn't 0, into task->motion, replacing
 * what it held unfreed. set task's step, draw and destructor from desc, then
 * call init with arg. returns task, or null when the record can't be allocated:
 * task_kill then makes task destroy itself on its next step */
struct task *task_apply_desc(struct task *task, const struct task_desc *desc,
                             void *arg);

#endif

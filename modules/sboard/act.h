/* Sand Hill's act tasks, the children of a parent that lives as long as the
 * act */
#ifndef SBOARD_ACT_H
#define SBOARD_ACT_H

#include "system/task.h"
#include "system/task_desc.h"

/* create the parent at level 3. its init remembers the parent, then makes
 * the act's tasks as its children. they are the sand worms, the rolling
 * rocks' watchers, the gate combo and the sun's lens flare. the parent runs
 * them each step, and destroys itself and them once the stage or act
 * changes. it stays remembered after that. returns the parent, or NULL when
 * none can be made */
struct task *act_parent_create(void);
/* destroy the last parent made and its tasks, and forget the parent.
 * act_parent_kill destroys them on the parent's next step, and
 * act_parent_destroy at once. the parent must still live. each does nothing
 * while no parent is remembered */
void act_parent_kill(void);
void act_parent_destroy(void);
/* create a task of desc with arg as the remembered parent's child, and
 * return it. the parent must still live. returns NULL when no parent is
 * remembered, or when the task can't be made */
struct task *act_task_create(const struct task_desc *desc, void *arg);

#endif

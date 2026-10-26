/* the Chao race's scenery task, whose children are the act's fixtures */
#ifndef AL_RACE_SCENERY_H
#define AL_RACE_SCENERY_H
#include <ninja.h>

#include "system/task.h"
#include "system/task_desc.h"

/* scenery_create makes the scenery task, which creates the act's fixtures as
 * it starts. scenery_destroy kills it, where there is one, and forgets it */
void scenery_create(void);
void scenery_destroy(void);
/* create a fixture from desc and arg as the scenery's child, and return its
 * task. null without the scenery task */
struct task *scenery_add(const struct task_desc *desc, void *arg);
/* place a fixture's work at object's position, unturned and at unit scale */
void scenery_place(struct task_work *work, NJS_OBJECT *object);

#endif

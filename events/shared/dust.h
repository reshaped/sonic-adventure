/* an event's dust: a task that puffs dust at its place, every interval
 * steps */
#ifndef EVENT_DUST_H
#define EVENT_DUST_H
#include <ninja.h>
#include <sg_xpt.h>

#include "effect/dust.h"
#include "system/task.h"

/* the dust task's work, in place of struct task_work. pos lies where
 * task_work keeps it */
struct event_dust_work {
    Uint8 unk_00[16];
    /* each puff's params, but pos. create allocates them, and the destructor
     * frees them */
    struct dust_params *params;
    Sint32 timer; /* steps to the next puff */
    Sint32 interval; /* the steps from one puff to the next */
    Uint8 unk_1c[4];
    NJS_POINT3 pos;
};

/* the work of a task from event_dust_create */
#define DUST_WORK(task) ((struct event_dust_work *)(task)->work)

/* create dust with the default params, white and still, or return null
 * when task_create fails. its timer and interval start at 0, so it puffs
 * only once the event sets them. a failed heap_alloc is unchecked */
struct task *event_dust_create(void);

#endif

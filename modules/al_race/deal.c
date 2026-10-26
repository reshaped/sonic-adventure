/* the deal, which collects the race's items and shuffles them out to the
 * lanes */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_race/deal.h"
#include "al_race/racer.h"
#include "al_race/scenery.h"
#include "debug/editor.h"
#include "system/shuffle.h"
#include "system/task.h"
#include "system/task_desc.h"

struct deal_item deal_items[DEAL_ITEMS];

static const struct task_desc deal_desc = {
    sizeof(struct deal_work), 0, deal_start, deal_step, NULL, deal_destroy};

Sint8 deal_state = DEAL_IDLE;
Sint8 deal_count = 0;
/* the open of the last deal_create made in DEAL_IDLE */
static Sint8 deal_open = 0;

struct task *deal_create(Sint8 open)
{
    struct task *task;

    if (deal_state) {
        task = NULL;
    } else {
        deal_open = open;
        task = scenery_add(&deal_desc, NULL);
    }
    return task;
}

Bool deal_add(struct task *task, void (*set)(struct task *, int), Sint8 kind)
{
    /* deal_count holds 0 to DEAL_ITEMS, so widening it as signed, as the
     * image does, loses nothing */
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    int count = deal_count;
    int result = 0;
    struct deal_item *entry;

    if (!deal_open || count >= DEAL_ITEMS) {
        task_kill(task);
    } else {
        switch (deal_state) {
        case DEAL_IDLE:
            break;
        case DEAL_OPEN:
            entry = &deal_items[count];
            entry->task = task;
            entry->set = set;
            entry->kind = kind;
            deal_count++;
            result = 1;
            break;
        case DEAL_DEALT:
            task_kill(task);
            break;
        }
    }
    return result;
}

int deal_take(NJS_POINT3 *pos, int lane)
{
    int result = 0;
    struct deal_item *entry;
    struct task *task;
    void (*set)(struct task *, int);

    if (deal_state == DEAL_DEALT && pos && lane >= 0 && lane < RACE_LANES) {
        entry = &deal_items[lane];
        task = entry->task;
        if (task) {
            *pos = task->work->pos;
            set = entry->set;
            result = 1;
            if (set) {
                set(task, lane);
            }
        }
    }
    return result;
}

void deal_start(struct task *task, void *arg)
{
    struct deal_work *work;

    deal_count = 0;
    deal_state = DEAL_OPEN;
    work = (struct deal_work *)task->motion;
    work->mode = 0;
}

void deal_step(struct task *task)
{
    /* deal_count holds 0 to DEAL_ITEMS, so widening it as signed, as the
     * image does, loses nothing */
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    int count = deal_count;
    struct deal_work *work = (struct deal_work *)task->motion;
    struct deal_item *entries;
    struct deal_item *entry;

    switch (work->mode) {
    case 0:
        if (count > 0 && !edit_mode) {
            entries = deal_items;
            for (entry = &entries[count]; count < RACE_LANES;
                 count++, entry++) {
                entry->task = NULL;
            }
            deal_count = count;
            deal_state = DEAL_DEALT;
            array_shuffle(entries, count, sizeof(struct deal_item));
            work->mode = 1;
        }
        break;
    case 1:
        break;
    }
}

void deal_destroy(struct task *task)
{
    deal_state = DEAL_IDLE;
}

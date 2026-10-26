/* Chaos 0's hints: to use the homing attack, once the player has lost a
 * life, and where to hit him, once the fight runs long */
#include <sg_xpt.h>

#include "b_chaos0/hint.h"
#include "chaos/chaos.h"
#include "draw/motion.h"
#include "event/script.h"
#include "object/tikal.h"
#include "stage/stage.h"
#include "system/task.h"

#define TEXT_1_LENGTH 69
#define TEXT_2_LENGTH 26
#include "b_chaos0/hint/text.inc"

/* the hints, in the order they're tested in. once the wait is over, each
 * step tries only the last hint that's due, which shows unless it has
 * already */
enum chaos0_hint { HINT_HOMING, HINT_CORE };

/* no hint shows within HINT_WAIT steps of the last. HINT_CORE is due once
 * the hints' task has run more than CORE_TIME steps, while his health is
 * still CORE_HEALTH or more */
#define HINT_WAIT 600
#define CORE_TIME 3600
#define CORE_HEALTH 3.0F

#define HOMING_VOICE 181
#define CORE_VOICE 182

struct hint_vars {
    Uint32 wait; /* the steps before another hint can show */
    Sint32 steps; /* the steps the task has run */
};

#define HINT_VARS(work) ((struct hint_vars *)&(work)->vars)

/* TRUE once each hint has shown, by enum chaos0_hint. the hints use two of
 * its four */
static Bool chaos0_hint_shown[4] = {FALSE};

static const struct script_page chaos0_homing_pages[2] = {
    {TEXT_1, 180},
    {NULL, 0},
};

static const struct script_page chaos0_core_pages[2] = {
    {TEXT_2, 120},
    {NULL, 0},
};

void chaos0_hint_step(struct task *task)
{
    struct task_work *work = task->work;
    int hint = -1;

    HINT_VARS(work)->steps++;
    if (HINT_VARS(work)->wait > 0) {
        HINT_VARS(work)->wait--;
        return;
    }
    if (stage_get_retries() >= 1) {
        hint = HINT_HOMING;
    }
    if (HINT_VARS(work)->steps > CORE_TIME &&
        chaos_record->health >= CORE_HEALTH) {
        hint = HINT_CORE;
    }
    if (hint < 0) {
        return;
    }
    if (chaos0_hint_shown[hint]) {
        return;
    }
    chaos0_hint_shown[hint] = TRUE;
    HINT_VARS(work)->wait = HINT_WAIT;
    switch (hint) {
    case HINT_HOMING:
        tikal_hint_create((struct script_page *)chaos0_homing_pages,
                          HOMING_VOICE);
        break;
    case HINT_CORE:
        tikal_hint_create((struct script_page *)chaos0_core_pages, CORE_VOICE);
        break;
    }
}

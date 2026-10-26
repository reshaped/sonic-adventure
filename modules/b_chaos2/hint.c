/* Chaos 2's hints: hit his head when he is open, hit his core by gliding or
 * spinning rather than punching, and punch his drops back */
#include <sg_xpt.h>

#include "b_chaos2/hint.h"
#include "chaos/chaos.h"
#include "draw/motion.h"
#include "event/script.h"
#include "object/tikal.h"
#include "stage/stage.h"
#include "system/task.h"

#define TEXT_1_LENGTH 43
#define TEXT_2_LENGTH 61
#define TEXT_3_LENGTH 61
#include "b_chaos2/hint/text.inc"

/* the hints, in the order the step tests them. once the wait is over, the
 * step shows the last of them that is due and hasn't shown yet */
enum chaos2_hint { HINT_HEAD, HINT_CORE, HINT_DROPS, HINTS };

/* no hint shows within HINT_WAIT steps of the last. HINT_HEAD is due once
 * the player has lost a life. it is due too once the hints' task has run
 * more than HINT_TIME steps while his health is still STRONG_HEALTH or
 * more. HINT_CORE is due once both hold. HINT_DROPS is due once the player
 * has lost a life and his health is down to WEAK_HEALTH */
#define HINT_WAIT 600
#define HINT_TIME 3600
#define STRONG_HEALTH 4.0F
#define WEAK_HEALTH 2.0F

/* the steps each hint's page shows for */
#define PAGE_STEPS 180

#define HEAD_VOICE 183
#define CORE_VOICE 843
#define DROPS_VOICE 184

struct hint_vars {
    Uint32 wait; /* the steps before another hint can show */
    Sint32 steps; /* the steps the task has run */
};

#define HINT_VARS(work) ((struct hint_vars *)&(work)->vars)

/* TRUE once each hint has shown, by enum chaos2_hint */
static Bool chaos2_hint_shown[HINTS] = {FALSE};

static const struct script_page chaos2_head_pages[2] = {
    {TEXT_1, PAGE_STEPS},
    {NULL, 0},
};

static const struct script_page chaos2_core_pages[2] = {
    {TEXT_2, PAGE_STEPS},
    {NULL, 0},
};

static const struct script_page chaos2_drops_pages[2] = {
    {TEXT_3, PAGE_STEPS},
    {NULL, 0},
};

void chaos2_hint_step(struct task *task)
{
    struct task_work *work = task->work;
    int hint = -1;

    HINT_VARS(work)->steps++;
    if (HINT_VARS(work)->wait > 0) {
        HINT_VARS(work)->wait--;
        return;
    }
    if (!chaos2_hint_shown[HINT_HEAD]) {
        if (stage_get_retries() >= 1 ||
            (HINT_VARS(work)->steps > HINT_TIME &&
             chaos_record->health >= STRONG_HEALTH)) {
            hint = HINT_HEAD;
        }
    }
    if (!chaos2_hint_shown[HINT_CORE]) {
        if (stage_get_retries() >= 1 && HINT_VARS(work)->steps > HINT_TIME &&
            chaos_record->health >= STRONG_HEALTH) {
            hint = HINT_CORE;
        }
    }
    if (!chaos2_hint_shown[HINT_DROPS]) {
        if (stage_get_retries() >= 1 && chaos_record->health <= WEAK_HEALTH) {
            hint = HINT_DROPS;
        }
    }
    if (hint >= 0) {
        chaos2_hint_shown[hint] = TRUE;
        HINT_VARS(work)->wait = HINT_WAIT;
        switch (hint) {
        case HINT_HEAD:
            tikal_hint_create((struct script_page *)chaos2_head_pages,
                              HEAD_VOICE);
            break;
        case HINT_CORE:
            tikal_hint_create((struct script_page *)chaos2_core_pages,
                              CORE_VOICE);
            break;
        case HINT_DROPS:
            tikal_hint_create((struct script_page *)chaos2_drops_pages,
                              DROPS_VOICE);
            break;
        }
    }
}

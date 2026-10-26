/* Chaos 6's hints, which Tikal gives. Sonic's and Knuckles' say that Chaos
 * can be frozen and what freezes him. Big's say where to cast the lure and
 * when */
#include <sg_xpt.h>

#include "b_chaos6/hint.h"
#include "chaos/chaos.h"
#include "draw/motion.h"
#include "event/script.h"
#include "object/tikal.h"
#include "player/player.h"
#include "stage/stage.h"
#include "system/task.h"

#define TEXT_1_LENGTH 57
#define TEXT_2_LENGTH 51
#define TEXT_3_LENGTH 69
#define TEXT_4_LENGTH 51
#define TEXT_5_LENGTH 32
#define TEXT_6_LENGTH 49
#include "b_chaos6/hint/text.inc"

/* the hints, in the order each step tests them. a step takes the last one
 * due, and shows it unless it has shown already */
enum chaos6_hint { HINT_FREEZE, HINT_SATELLITES, HINT_LURE, HINT_NEAR };

/* the steps after a hint before the next can show */
#define HINT_WAIT 600
/* the freeze, satellites and near hints are each due once the hints' task
 * has run more than its _TIME steps. the lure hint is due from the first
 * step. the freeze and satellites hints also need the boss's health at
 * least at their _HEALTH. after a retry, the freeze hint, at any health, and
 * the near hint are due from the first step */
#define FREEZE_TIME 5400
#define FREEZE_HEALTH 6.0F
#define SATELLITES_TIME 7200
#define SATELLITES_HEALTH 3.0F
#define NEAR_TIME 7200

#define FREEZE_VOICE 186
#define SATELLITES_VOICE 845
#define LURE_VOICE 848
#define NEAR_VOICE 850

struct hint_vars {
    Uint32 wait; /* the steps left before the next hint can show */
    Sint32 steps; /* the steps the task has run */
};

#define HINT_VARS(work) ((struct hint_vars *)&(work)->vars)

static const struct script_page chaos6_freeze_pages[2] = {
    {TEXT_1, 180},
    {NULL, 0},
};

static const struct script_page chaos6_satellites_pages[3] = {
    {TEXT_2, 180},
    {TEXT_3, 180},
    {NULL, 0},
};

static const struct script_page chaos6_lure_pages[2] = {
    {TEXT_4, 180},
    {NULL, 0},
};

static const struct script_page chaos6_near_pages[3] = {
    {TEXT_5, 120},
    {TEXT_6, 180},
    {NULL, 0},
};

/* chaos6_hint_shown[hint] is TRUE once hint has shown */
static Sint8 chaos6_hint_shown[4] = {FALSE};

Sint8 chaos6_hint_waiting;

void chaos6_hint_step(struct task *task)
{
    struct task_work *work = task->work;
    int hint = -1;

    HINT_VARS(work)->steps++;
    if (HINT_VARS(work)->wait > 0) {
        HINT_VARS(work)->wait--;
        chaos6_hint_waiting = TRUE;
        return;
    }
    chaos6_hint_waiting = FALSE;
    switch (get_character()) {
    case CHARACTER_SONIC:
    case CHARACTER_KNUCKLES:
        if (stage_get_retries() >= 1 ||
            (HINT_VARS(work)->steps > FREEZE_TIME &&
             chaos_record->health >= FREEZE_HEALTH)) {
            hint = HINT_FREEZE;
        }
        if (HINT_VARS(work)->steps > SATELLITES_TIME &&
            chaos_record->health >= SATELLITES_HEALTH) {
            hint = HINT_SATELLITES;
        }
        break;
    case CHARACTER_BIG:
        hint = HINT_LURE;
        if (stage_get_retries() >= 1 || HINT_VARS(work)->steps > NEAR_TIME) {
            hint = HINT_NEAR;
        }
        break;
    }
    if (hint < 0) {
        return;
    }
    if (chaos6_hint_shown[hint]) {
        return;
    }
    chaos6_hint_shown[hint] = TRUE;
    HINT_VARS(work)->wait = HINT_WAIT;
    switch (hint) {
    case HINT_FREEZE:
        tikal_hint_create((struct script_page *)chaos6_freeze_pages,
                          FREEZE_VOICE);
        break;
    case HINT_SATELLITES:
        tikal_hint_create((struct script_page *)chaos6_satellites_pages,
                          SATELLITES_VOICE);
        break;
    case HINT_LURE:
        tikal_hint_create((struct script_page *)chaos6_lure_pages, LURE_VOICE);
        break;
    case HINT_NEAR:
        tikal_hint_create((struct script_page *)chaos6_near_pages, NEAR_VOICE);
        break;
    }
}

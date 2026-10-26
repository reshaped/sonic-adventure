/* Tikal's hint in the Perfect Chaos fight */
#include <sg_xpt.h>

#include "b_chaos7/hint.h"
#include "event/script.h"
#include "object/tikal.h"
#include "stage/stage.h"
#include "system/task.h"

#define TEXT_1_LENGTH 55
#define TEXT_2_LENGTH 30
#include "b_chaos7/hint/text.inc"

/* a hint can show once the task has run more than HINT_DELAY steps. after a
 * hint shows, the task waits HINT_WAIT steps before it looks again */
#define HINT_DELAY 60
#define HINT_WAIT 720

#define VOICE_HINT 855

struct hint_vars {
    Uint32 wait;
    Sint32 steps;
};

#define VARS(work) ((struct hint_vars *)&(work)->vars)

static const struct script_page chaos7_hint_pages[] = {
    {TEXT_1, 240},
    {TEXT_2, 120},
    {NULL, 0},
};

/* a flag for each hint, set once that hint has shown. of the four, only hint 0
 * is used */
static Sint32 chaos7_hints_shown[4] = {0};

/* show the hint on how to hit Chaos once the stage has counted a retry.
 * Tikal's light brings it to player 0. each hint shows once while the module
 * stays loaded */
void chaos7_hint_step(struct task *task)
{
    struct task_work *work = task->work;
    int hint = -1;

    VARS(work)->steps++;
    if (VARS(work)->wait > 0) {
        VARS(work)->wait--;
        return;
    }
    if (stage_get_retries() >= 1 && VARS(work)->steps > HINT_DELAY) {
        hint = 0;
    }
    if (hint >= 0) {
        if (chaos7_hints_shown[hint]) {
            return;
        }
        chaos7_hints_shown[hint] = 1;
        VARS(work)->wait = HINT_WAIT;
        switch (hint) {
        case 0:
            tikal_hint_create((struct script_page *)chaos7_hint_pages,
                              VOICE_HINT);
            break;
        }
    }
}

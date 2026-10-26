/* making characters playable, and the notice of each */
#include <sg_xpt.h>
#include <stdio.h>

#include "event/play.h"
#include "event/script.h"
#include "player/player.h"
#include "progress/playable.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "system/task.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 6
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 6
#define TEXT_7_LENGTH 12
#define TEXT_8_LENGTH 5
#define TEXT_9_LENGTH 5
#define TEXT_10_LENGTH 8
#define TEXT_11_LENGTH 3
#define TEXT_12_LENGTH 5
#define TEXT_13_LENGTH 3
#include "progress/playable/text.inc"

/* the notice shows after this many steps in a row with no message busy and
 * no event in control */
#define NOTICE_DELAY 20

/* the bytes allocated for the notice's line */
#define LINE_SIZE 64

struct playable_name {
    Sint32 character; /* or -1, to end the table */
    char *english;
    char *japanese; /* in Shift JIS */
};

enum notice_mode {
    NOTICE_WAIT, /* for the delay to run out */
    NOTICE_SHOW /* until the message is gone */
};

struct notice_vars {
    Sint32 timer;
    char **lines; /* on the heap until the message is gone */
    struct playable_name *name;
};

#define VARS(work) ((struct notice_vars *)&(work)->vars)

static void notice_step(struct task *task);

static Bool notice_running = FALSE;

/* the characters that can be made playable */
static struct playable_name playable_names[] = {
    {CHARACTER_SONIC, TEXT_8, TEXT_1},
    {CHARACTER_TAILS, TEXT_9, TEXT_2},
    {CHARACTER_KNUCKLES, TEXT_10, TEXT_3},
    {CHARACTER_AMY, TEXT_11, TEXT_4},
    {CHARACTER_E102, TEXT_12, TEXT_5},
    {CHARACTER_BIG, TEXT_13, TEXT_6},
    {-1, NULL, NULL},
};

/* save the story as the notice's task is destroyed, whether or not its
 * message has shown */
static void notice_destroy(struct task *task)
{
    story_save();
    notice_running = FALSE;
}

static void notice_draw(struct task *task)
{
}

static void notice_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case NOTICE_WAIT:
        if (message_busy() || event_in_control()) {
            VARS(work)->timer = NOTICE_DELAY;
        } else if (--VARS(work)->timer == 0) {
            struct playable_name *name = VARS(work)->name;
            /* neither allocation is checked before the line is written */
            char *line = heap_alloc(LINE_SIZE);
            char **lines = (char **)heap_alloc(2 * sizeof(*lines));

            lines[0] = line;
            lines[1] = NULL;
            /* a leading tab centers the Japanese name and the notice's text */
            sprintf(line, "\t%s " TEXT_7, name->japanese);
            sound_play(SE_NOTICE, 0, 0, 0);
            message_show_timed(lines);
            VARS(work)->lines = lines;
            work->mode = NOTICE_SHOW;
        }
        break;
    case NOTICE_SHOW:
        if (!message_busy()) {
            char **lines = VARS(work)->lines;

            heap_free(lines[0]);
            heap_free((void *)lines);
            task_kill(task);
        }
        break;
    }
}

static struct task *notice_create(struct playable_name *name)
{
    struct task *task;
    struct task_work *work;

    task = task_create(TASK_WORK, 5, notice_step);
    if (task != NULL) {
        task->step = notice_step;
        task->draw = notice_draw;
        task->destructor = notice_destroy;
        work = task->work;
        VARS(work)->name = name;
        VARS(work)->timer = NOTICE_DELAY;
        work->mode = NOTICE_WAIT;
        notice_running = TRUE;
    }
    return task;
}

struct task *playable_unlock(int character)
{
    struct playable_name *name;

    name = playable_names;
    if (name->character >= 0) {
        while (name->character != character) {
            name++;
            /* @hack spelling: a continue, then a break, which loads
             * name->character into R2, then R1 (0x8c0ad878, 0x8c0ad87e);
             * inverted, as a break alone, they take R3 and R1 and 5 of 40
             * words differ */
            if (name->character >= 0) {
                continue;
            }
            break;
        }
    }
    if (name->character < 0) {
        return NULL;
    }
    if (playable_check(character)) {
        return NULL;
    }
    playable_set(character);
    return notice_create(name);
}

Bool playable_notice_active(void)
{
    return notice_running;
}

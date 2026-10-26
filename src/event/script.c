/* message scripts and the message window. a message waits in a runner task
 * until the window is free, then shows on a thread of its own, which yields
 * each step */
#include <sg_xpt.h>
#include <shinobi.h>
#include <sofdec.h>

#include "event/play.h"
#include "event/script.h"
#include "progress/progress.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/bitmap.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/thread.h"

/* script_run's result for a branch that passes in a row without runs */
#define NO_RUNS ((void *)-1)

/* what a table's runs hold */
enum script_runs { RUNS_PAGES, RUNS_LINES };

#define TIMED_STEPS 60

/* a stage module's script table, at its address once the module loads */
#define MODULE_SCRIPTS(address) ((struct script_row *)(address))

/* a message waiting for the window, kept in the runner task's extra */
struct message_request {
    Uint8 *shown; /* the flag message_set_shown gave it, or null */
    void *run; /* what its thread runs, on arg */
    void *arg;
    Bool ready; /* it holds the window */
    Bool voice; /* a voice says it */
};

static void script_set_flag(int flag);
static void *script_run(struct script_pages *pick, Sint16 *script, void *run,
                        enum script_runs kind);
static Bool script_skip(Bool match, Sint16 **script, void **run,
                        enum script_runs kind);
static Bool events_match(Sint16 *list);
static Bool flags_match(Sint16 *list);
static Sint16 *script_next_branch(Sint16 *script);
static void *script_next_run(void *run, enum script_runs kind);
static void run_pages(struct script_page *pages);
static void run_lines(char **lines);
static void run_timed(char **lines);
static void message_start(void *run, void *arg, Bool voice);
static void message_end(void);
static void message_runner_step(struct task *task);

/* wait count steps, or until the message is cut short. each step yields
 * the thread and holds lives_shown at -1, and a frame's first draws the
 * window */
/* @hack spelling: the count taken down as steps_ - 1, which keeps the pages
 * in R13 for run_pages' loop (MOV R4,R13 at 0x8c062c86); with --, the pages
 * take R14, and 9 of the 366 words of the five functions that wait differ:
 * message_fade_text, message_fade_back, message_wait_button, run_pages and
 * run_timed */
#define WAIT_STEPS(window, count)                                              \
    do {                                                                       \
        int steps_;                                                            \
                                                                               \
        if ((steps_ = (count)) != 0 && !message_cut_short) {                   \
            do {                                                               \
                if (!frame_step) {                                             \
                    bitmap_draw(window);                                       \
                }                                                              \
                thread_yield();                                                \
                steps_ = steps_ - 1;                                           \
                lives_shown = -1;                                              \
            } while (steps_ != 0 && !message_cut_short);                       \
        }                                                                      \
    } while (0)

/* each stage's script table, or null where script_pick runs the field's */
static struct script_row *stage_scripts[STAGE_ROWS] = {
    NULL, /* STAGE_PRACTICE */
    MODULE_SCRIPTS(0x0c911738), /* STAGE_BEACH */
    MODULE_SCRIPTS(0x0ca158f0), /* STAGE_WINDY */
    MODULE_SCRIPTS(0x0c9f8688), /* STAGE_TWINKLE */
    MODULE_SCRIPTS(0x0c913a20), /* STAGE_HIGHWAY */
    MODULE_SCRIPTS(0x0cadd260), /* STAGE_MOUNTAIN */
    MODULE_SCRIPTS(0x0cae9848), /* STAGE_SKY_DECK */
    MODULE_SCRIPTS(0x0ca5d308), /* STAGE_RUIN */
    MODULE_SCRIPTS(0x0cac3910), /* STAGE_ICECAP */
    MODULE_SCRIPTS(0x0cb1a4a8), /* STAGE_CASINO */
    MODULE_SCRIPTS(0x0c919300), /* STAGE_FINAL */
    NULL, /* 11, unnamed */
    MODULE_SCRIPTS(0x0caa4b58), /* STAGE_SHELTER */
    NULL, /* 13, unnamed */
    NULL, /* 14, unnamed */
    NULL, /* STAGE_CHAOS0 */
    NULL, /* STAGE_CHAOS2 */
    NULL, /* STAGE_CHAOS4 */
    NULL, /* STAGE_CHAOS6 */
    NULL, /* STAGE_CHAOS7 */
    NULL, /* STAGE_EGGMOBILE1 */
    NULL, /* STAGE_EGGMOBILE2 */
    NULL, /* STAGE_EGGMOBILE3 */
    NULL, /* STAGE_EGGMANROBO */
    NULL, /* STAGE_E101 */
    NULL, /* STAGE_E101_R */
    NULL, /* STAGE_STATION_SQUARE */
    NULL, /* STAGE_STATION_SQUARE_2 */
    NULL, /* STAGE_STATION_SQUARE_3 */
    NULL, /* STAGE_EGG_CARRIER */
    NULL, /* STAGE_EGG_CARRIER_2 */
    NULL, /* STAGE_EGG_CARRIER_3 */
    NULL, /* STAGE_EGG_CARRIER_INSIDE */
    NULL, /* STAGE_MYSTIC_RUINS */
    NULL, /* STAGE_PAST */
    NULL, /* STAGE_CART */
    NULL, /* STAGE_SHOOTING1 */
    NULL, /* STAGE_SHOOTING2 */
    NULL, /* STAGE_SUNABOARD */
    MODULE_SCRIPTS(0x0cb812b0), /* STAGE_ALIFE_SS */
    MODULE_SCRIPTS(0x0cb80a88), /* STAGE_ALIFE_EG */
    MODULE_SCRIPTS(0x0cb81300), /* STAGE_ALIFE_MR */
    NULL, /* STAGE_ALIFE_RACE */
    NULL, /* STAGE_NONE */
};
/* the tables script_set_table and script_set_lines_table set */
static struct script_row *field_scripts = NULL;
static struct script_row *lines_scripts = NULL;
/* set from a message's first page until its thread ends */
static Bool message_window_open = FALSE;
/* set to end the message showing early. the runner clears it as the next
 * message's thread starts */
static Bool message_cut_short = FALSE;

/* whether a voice says the message holding the window */
static Bool message_voiced;
/* set while a message holds the window */
static Bool message_holding;
/* the flag message_set_shown gives the next message started, or null */
static Uint8 *message_shown_next;
/* the flag of the message holding the window, or null */
static Uint8 *message_shown;
static struct bitmap message_window;

void message_show_timed(char **lines)
{
    message_start((void *)run_timed, (void *)lines, FALSE);
}

void message_set_shown(Uint8 *shown)
{
    message_shown_next = shown;
}

void script_set_lines_table(struct script_row *table)
{
    lines_scripts = table;
    message_holding = FALSE;
    message_voiced = FALSE;
    message_cut_short = FALSE;
}

void script_set_table(struct script_row *table)
{
    field_scripts = table;
    message_holding = FALSE;
    message_voiced = FALSE;
    message_cut_short = FALSE;
}

Bool script_pick(int row, struct script_pages *pick)
{
    struct script_row *table;
    Sint16 *script;
    void *runs;
    void *run;

    if (stage_scripts[STAGE_OF(stage_get_stage_act())] != NULL) {
        table = stage_scripts[STAGE_OF(stage_get_stage_act())];
    } else {
        table = field_scripts;
    }
    if (table != NULL) {
        script = table[row].script;
        runs = table[row].runs;
        pick->voice = -1;
        pick->flag = -1;
        pick->pages = NULL;
        if ((run = script_run(pick, script, runs, RUNS_PAGES)) != NULL) {
            pick->pages = run == NO_RUNS ? NULL : run;
            return TRUE;
        }
    }
    return FALSE;
}

Bool script_pick_lines(int row, struct script_lines *lines)
{
    struct script_pages pick;
    struct script_row *table;
    Sint16 *script;
    void *runs;

    table = lines_scripts;
    if (table != NULL) {
        script = table[row].script;
        runs = table[row].runs;
        lines->lines = (char **)script_run(&pick, script, runs, RUNS_LINES);
        /* @bug uninitialized: nothing sets pick.flag but the script's
         * SCRIPT_FLAG, so without one the flag is a stack word no path wrote
         * (0x8c062484), and script_show_lines sets or clears the flag that
         * word names where it isn't below 0 */
        lines->flag = pick.flag;
        if (lines->lines) {
            if ((void *)lines->lines == NO_RUNS) {
                lines->lines = NULL;
            }
            return TRUE;
        }
    }
    return FALSE;
}

void script_show(struct script_pages *pick)
{
    Bool voice;

    if (pick->voice > -1) {
        stream_request_voice(pick->voice);
        voice = TRUE;
    } else {
        voice = FALSE;
    }
    if (pick->pages != NULL) {
        message_start((void *)run_pages, pick->pages, voice);
    }
    script_set_flag(pick->flag);
}

void script_show_lines(struct script_lines *lines)
{
    if (lines->lines) {
        message_start((void *)run_lines, (void *)lines->lines, FALSE);
    }
    script_set_flag(lines->flag);
}

Bool message_busy(void)
{
    Sint32 state = stream_voice_state();

    if (state == ADXT_STAT_PLAYEND) {
        stream_stop_voice();
        return message_holding;
    }
    if (state == ADXT_STAT_STOP) {
        return message_holding;
    }
    return TRUE;
}

void message_cut(void)
{
    message_cut_short = TRUE;
    stream_stop_voice();
}

static void script_set_flag(int flag)
{
    if (flag > -1) {
        if (flag & SCRIPT_ON) {
            event_flags[flag & ~SCRIPT_ON] = 1;
        } else {
            event_flags[flag] = 0;
        }
    }
}

/* whether the character the player plays is one of the mask's */
static int character_in(Sint16 mask)
{
    int bit = 1 << get_character();

    return mask & bit;
}

/* run script, and return the run of its first branch that passes. run is
 * the first branch's run, and each later branch's follows the one before.
 * return NO_RUNS when a branch passes with run null, and null when none
 * passes. each SCRIPT_VOICE and SCRIPT_FLAG read sets pick's voice or flag,
 * in a branch that fails too */
static void *script_run(struct script_pages *pick, Sint16 *script, void *run,
                        enum script_runs kind)
{
    Bool match;

    if (script != NULL) {
        while (*script != SCRIPT_BRANCH && *script != SCRIPT_END) {
            switch (*script) {
            case SCRIPT_VOICE:
                pick->voice = *++script;
                break;
            case SCRIPT_EVENTS:
                match = events_match(++script);
                if (!script_skip(match, &script, &run, kind)) {
                    return NULL;
                }
                break;
            case SCRIPT_FLAGS:
                match = flags_match(++script);
                if (!script_skip(match, &script, &run, kind)) {
                    return NULL;
                }
                break;
            case SCRIPT_CHARACTERS:
                if (!character_in(*++script)) {
                    script = script_next_branch(++script);
                    if (script == NULL) {
                        return NULL;
                    }
                    run = script_next_run(run, kind);
                    /* @hack redundant: a break where the case breaks anyway,
                     * which takes the next word through R3 (0x8c0626d6 to
                     * 0x8c0626dc); without it, R2, and 8 of 124 words differ */
                    break;
                }
                break;
            case SCRIPT_FLAG:
                pick->flag = *++script;
                break;
            default:
                return NULL;
            }
            script++;
        }
    }
    if (run != NULL) {
        return run;
    }
    return NO_RUNS;
}

/* move *script past the list of events or flags it is on, whose test match
 * gives. a test that passes leaves *script on the list's last entry. one
 * that fails moves *script to the next SCRIPT_BRANCH and *run to that
 * branch's run. return FALSE when a test fails with no branch left */
static Bool script_skip(Bool match, Sint16 **script, void **run,
                        enum script_runs kind)
{
    if (!match) {
        /* NOLINTBEGIN(bugprone-inc-dec-in-conditions) */
        if ((*script = script_next_branch(++*script)) != NULL) {
            *run = script_next_run(*run, kind);
        } else {
            return FALSE;
        }
        /* NOLINTEND(bugprone-inc-dec-in-conditions) */
    } else {
        while (*++*script >= 0) {
        }
        --*script;
    }
    return TRUE;
}

/* TRUE when any event in list is as its entry says. list ends at a word
 * below 0, after at least one entry */
static Bool events_match(Sint16 *list)
{
    int entry;

    do {
        entry = *list;
        if (entry & SCRIPT_ON) {
            if (event_was_played(entry & ~SCRIPT_ON)) {
                return TRUE;
            }
        } else {
            if (!event_was_played(entry)) {
                return TRUE;
            }
        }
    } while (*++list >= 0);
    return FALSE;
}

/* TRUE when any flag in list is as its entry says. list ends as
 * events_match's does */
static Bool flags_match(Sint16 *list)
{
    int entry;

    do {
        entry = *list;
        if (entry & SCRIPT_ON) {
            if (event_flags[entry & ~SCRIPT_ON]) {
                return TRUE;
            }
        } else {
            if (!event_flags[entry]) {
                return TRUE;
            }
        }
    } while (*++list >= 0);
    return FALSE;
}

static Sint16 *script_next_branch(Sint16 *script)
{
    while (*script != SCRIPT_END) {
        if (*script == SCRIPT_BRANCH) {
            return script;
        }
        script++;
    }
    return NULL;
}

/* the run after run, past its null, or null for a null run. run has to
 * hold a page or a line before its null: the search starts at its second */
static void *script_next_run(void *run, enum script_runs kind)
{
    struct script_page *page;
    char **line;

    if (run == NULL) {
        return NULL;
    }
    switch (kind) {
    case RUNS_PAGES:
        page = run;
        while ((++page)->text != NULL) {
        }
        return page + 1;
    case RUNS_LINES:
        line = (char **)run;
        while (*++line != NULL) {
        }
        return (void *)(line + 1);
    default:
        return NULL;
    }
}

/* add speed to the window's text alpha each step, for count steps, and keep
 * it within 0 to 0xff */
static void message_fade_text(struct bitmap *window, int count, int speed)
{
    int alpha;
    int i;

    for (i = count; i != 0; i--) {
        /* @hack spelling: the sum assigned inside the test, which puts the 0xff
         * arm's MOV R1,R2 in the BRA's slot (0x8c062946, 0x8c062948);
         * assigned before it, the slot holds a NOP, the function runs 2 bytes
         * long and 44 of 71 words differ */
        /* NOLINTBEGIN(readability-avoid-nested-conditional-operator) */
        window->text_color.argb.a =
            (alpha = window->text_color.argb.a + speed) < 0 ? 0
            : alpha > 0xff                                  ? 0xff
                                                            : alpha;
        /* NOLINTEND(readability-avoid-nested-conditional-operator) */
        WAIT_STEPS(window, 1);
    }
}

/* the same for the window's back */
static void message_fade_back(struct bitmap *window, int count, int speed)
{
    int alpha;
    int i;

    for (i = count; i != 0; i--) {
        /* @hack spelling: the sum assigned inside the test, as in
         * message_fade_text (0x8c062a06, 0x8c062a08); assigned before it, the
         * function runs 2 bytes long and 44 of 71 words differ */
        /* NOLINTBEGIN(readability-avoid-nested-conditional-operator) */
        window->back_color.argb.a =
            (alpha = window->back_color.argb.a + speed) < 0 ? 0
            : alpha > 0xff                                  ? 0xff
                                                            : alpha;
        /* NOLINTEND(readability-avoid-nested-conditional-operator) */
        WAIT_STEPS(window, 1);
    }
}

/* fade the window in: the back to half alpha, then the text to full */
static void message_open(struct bitmap *window)
{
    window->back_color.argb.a = 0;
    window->text_color.argb.a = 0;
    message_fade_back(window, 10, 12);
    window->back_color.argb.a = 0x80;
    message_fade_text(window, 10, 24);
    window->text_color.argb.a = 0xff;
}

/* fade the window out, the text first and then the back, and show it two
 * steps more */
static void message_close(struct bitmap *window)
{
    int steps;

    message_fade_text(window, 10, -24);
    window->text_color.argb.a = 0;
    message_fade_back(window, 10, -12);
    window->back_color.argb.a = 0;
    /* @hack redundant: 2 as a select whose condition is constant, which SHC
     * folds too late to drop the test of the count before the wait (MOV
     * #2,R13 and TST R13,R13 at 0x8c062ae6, 0x8c062ae8); with a plain 2, it
     * drops the test, the function runs 4 bytes short and 39 of 58 words
     * differ */
    /* NOLINTNEXTLINE(bugprone-branch-clone,misc-redundant-expression) */
    steps = 0.0F ? 2 : 2;
    if (steps != 0 && !message_cut_short) {
        do {
            if (!frame_step) {
                bitmap_draw(window);
            }
            thread_yield();
            steps--;
            lives_shown = -1;
        } while (steps != 0 && !message_cut_short);
    }
}

/* show text as the message's next page. the first page opens the window,
 * and each later one fades the old text out and the new text in */
static void message_show(struct bitmap *window, char *text)
{
    /* @hack spelling: fade copied from a constant, which SHC keeps in R13 and
     * tests (MOV #1,R13 at 0x8c062b48); set to TRUE, it folds the tests, the
     * function runs 8 bytes short and 40 of 62 words differ */
    Bool on = TRUE;
    Bool fade = on;

    if (!message_window_open) {
        bitmap_init_window(window);
        /* fade holds TRUE, which is BITMAP_EVENT's value. a voiced
         * message's window takes that mode, in which it hides while
         * messages_shown is clear */
        if (message_voiced) {
            window->mode = fade;
        }
        bitmap_print(window, text);
        bitmap_queue(window);
        if (fade) {
            message_open(window);
            message_window_open = fade;
        }
    } else {
        message_fade_text(window, 7, -32);
        bitmap_clear(window);
        bitmap_print(window, text);
        bitmap_queue(window);
        if (fade) {
            message_fade_text(window, 7, 32);
        }
    }
}

/* wait until pad 0 presses a jump or grab button, or the message is cut
 * short */
static void message_wait_button(struct bitmap *window)
{
    do {
        WAIT_STEPS(window, 1);
    } while (!(per_info[0]->press & (pad_jump_buttons | pad_grab_buttons)) &&
             !message_cut_short);
}

static void message_finish(struct bitmap *window)
{
    if (message_window_open) {
        message_close(window);
    }
    pause_enable();
}

/* a message thread's run. it shows each page for its steps, or until a
 * jump or grab button where its steps are 0, then closes the window.
 * pausing is off meanwhile */
static void run_pages(struct script_page *pages)
{
    char *text;

    message_holding = TRUE;
    pause_disable();
    while ((text = pages->text) != NULL && !message_cut_short) {
        message_show(&message_window, text);
        if (pages->steps) {
            WAIT_STEPS(&message_window, pages->steps);
        } else {
            message_wait_button(&message_window);
        }
        pages++;
    }
    message_finish(&message_window);
}

/* a message thread's run, as run_pages but for lines, each shown until a
 * jump or grab button */
static void run_lines(char **lines)
{
    char *text;

    message_holding = TRUE;
    pause_disable();
    while ((text = *lines) != NULL && !message_cut_short) {
        message_show(&message_window, text);
        message_wait_button(&message_window);
        lines++;
    }
    message_finish(&message_window);
}

/* a message thread's run, as run_pages but for lines, each shown for
 * TIMED_STEPS */
static void run_timed(char **lines)
{
    char *text;

    message_holding = TRUE;
    pause_disable();
    while ((text = *lines++) != NULL && !message_cut_short) {
        message_show(&message_window, text);
        WAIT_STEPS(&message_window, TIMED_STEPS);
    }
    message_finish(&message_window);
}

/* start a message: a runner task that waits for the window, then runs run
 * on arg on a thread of its own. voice is TRUE for a voiced message. the
 * message showing is cut short and the voice playing stops. a task or
 * request that can't be made drops the new message */
static void message_start(void *run, void *arg, Bool voice)
{
    struct task *task;
    struct message_request *request;
    Uint8 *shown;

    task = task_create(0, 6, message_runner_step);
    message_cut_short = TRUE;
    stream_stop_voice();
    if (task != NULL) {
        request = heap_calloc(1, sizeof(struct message_request));
        if (request != NULL) {
            shown = message_shown_next;
            if (shown != NULL) {
                request->shown = shown;
                *shown = 1;
                message_shown_next = NULL;
            }
            request->run = run;
            request->arg = arg;
            request->voice = voice;
            task->extra = request;
        } else {
            task_destroy(task);
        }
    }
}

/* the message thread's destructor. it releases the window, frees it for
 * the next message and clears the message's flag */
static void message_end(void)
{
    bitmap_clear_queue();
    if (message_window_open) {
        bitmap_release(&message_window);
        message_window_open = FALSE;
    }
    message_holding = FALSE;
    /* @hack spelling: the test and its two stores as an expression, which
     * saves no R14 (STS.L PR,@-R15 at 0x8c062ebc); as an if, the two stores
     * share one zero in R14, the function runs 8 bytes short and 31 of 31
     * words differ */
    message_shown && (*message_shown = 0, message_shown = NULL);
}

/* the runner's step. the message takes the window once no other holds it.
 * on a frame's first step after that, the runner starts the message's
 * thread and destroys itself */
static void message_runner_step(struct task *task)
{
    struct message_request *request = task->extra;
    struct thread *thread;

    if (request->ready && !frame_step) {
        message_cut_short = FALSE;
        thread = thread_create(request->run, request->arg, 0);
        thread_set_destructor(thread, message_end);
        task_destroy(task);
    }
    if (!message_holding) {
        if (request->shown != NULL) {
            message_shown = request->shown;
        }
        message_voiced = request->voice;
        message_holding = TRUE;
        request->ready = TRUE;
    }
}

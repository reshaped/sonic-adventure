/* the menus' screens: the menus' task, which holds the screen the menus are
 * at, and the links between the screens */
#include "advertise/screen.h"
#include "advertise/card_screen.h"
#include "advertise/character_select.h"
#include "advertise/drive_select.h"
#include "advertise/file_select.h"
#include "advertise/game_bug.h"
#include "advertise/mini_game_select.h"
#include "advertise/mode_select.h"
#include "advertise/options.h"
#include "advertise/sound.h"
#include "advertise/sound_test.h"
#include "advertise/textures.h"
#include "advertise/trial.h"
#include "advertise/trial_stage_select.h"
#include "hud/menu.h"
#include "main.h"
#include "stage/stage.h"
#include "system/task.h"

/* the screens' tasks start OPEN_STEPS steps after the menus do. screen_go's
 * switch comes at the last of CHANGE_STEPS steps of the menus' task after
 * the call */
#define OPEN_STEPS 130
#define CHANGE_STEPS 10

/* the work of the task that starts the screens' tasks: their list, and the
 * steps left */
struct open_work {
    const struct screen_entry *list;
    int timer;
};

/* each entrance's screens, with their links. the menus start at the first:
 * the card screen from the top, and the wheel on a return */
static const struct screen_entry top_screens[] = {
    {SCREEN_CARD, {SCREEN_NONE, SCREEN_DRIVE}},
    {SCREEN_DRIVE, {SCREEN_CARD, SCREEN_FILE}},
    {SCREEN_FILE, {SCREEN_DRIVE, SCREEN_MODE}},
    {SCREEN_MODE, {SCREEN_FILE, SCREEN_CHARACTER}},
    {SCREEN_OPTIONS, {SCREEN_MODE, SCREEN_SOUND_TEST}},
    {SCREEN_SOUND_TEST, {SCREEN_OPTIONS, SCREEN_NONE}},
    {SCREEN_CHARACTER, {SCREEN_MODE, SCREEN_TRIAL_STAGE}},
    {SCREEN_TRIAL_STAGE, {SCREEN_CHARACTER, SCREEN_NONE}},
    {SCREEN_GAME_BUG, {SCREEN_CHARACTER, SCREEN_CHARACTER}},
    {SCREEN_TRIAL_MINI_GAME, {SCREEN_CHARACTER, SCREEN_NONE}},
    {SCREEN_NONE, {SCREEN_NONE, SCREEN_NONE}},
};
static const struct screen_entry return_screens[] = {
    {SCREEN_CHARACTER, {SCREEN_MODE, SCREEN_GAME_BUG}},
    {SCREEN_CARD, {SCREEN_NONE, SCREEN_DRIVE}},
    {SCREEN_DRIVE, {SCREEN_CARD, SCREEN_FILE}},
    {SCREEN_FILE, {SCREEN_DRIVE, SCREEN_MODE}},
    {SCREEN_MODE, {SCREEN_FILE, SCREEN_CHARACTER}},
    {SCREEN_OPTIONS, {SCREEN_MODE, SCREEN_SOUND_TEST}},
    {SCREEN_SOUND_TEST, {SCREEN_OPTIONS, SCREEN_NONE}},
    {SCREEN_TRIAL_STAGE, {SCREEN_CHARACTER, SCREEN_NONE}},
    {SCREEN_GAME_BUG, {SCREEN_CHARACTER, SCREEN_CHARACTER}},
    {SCREEN_TRIAL_MINI_GAME, {SCREEN_CHARACTER, SCREEN_NONE}},
    {SCREEN_NONE, {SCREEN_NONE, SCREEN_NONE}},
};

/* each menu_entrance's list, indexed by the entrance */
static const struct screen_entry *screen_lists[] = {
    top_screens,
    return_screens,
    return_screens,
};
/* each screen's create and destroy, indexed by enum screen below
 * SCREEN_CLOSE */
static struct task *(*screen_creates[])(struct screen_links *links) = {
    NULL,
    character_select_create,
    file_select_create,
    trial_stage_select_create,
    drive_select_create,
    sound_test_create,
    card_screen_create,
    mode_select_create,
    options_create,
    game_bug_create,
    mini_game_select_create,
};
static void (*screen_destroys[])(void) = {
    NULL,
    character_select_destroy,
    file_select_destroy,
    trial_stage_select_destroy,
    drive_select_destroy,
    sound_test_destroy,
    card_screen_destroy,
    mode_select_destroy,
    options_destroy,
    game_bug_destroy,
    mini_game_select_destroy,
};

struct task *advertise_task;

static void screens_create(const struct screen_entry *entry);
static void screen_set(int screen);

static void screens_open_step(struct task *task)
{
    struct open_work *work = task->extra;

    if (--work->timer <= 0) {
        screens_create(work->list);
        task_destroy(task);
    }
}

static void screens_open(const struct screen_entry *list)
{
    struct task *task;
    struct open_work *work;

    /* @hack spelling: the task assigned in the test. SHC then tests it in R0
     * (TST R0,R0 at 0x8c901460), and copies it to R14 in the slot of the
     * BT/S. assigned first, it is copied to R14 and tested there, and 3 of 28
     * words differ */
    if ((task = task_create(0, 3, screens_open_step)) != NULL) {
        work = heap_calloc(1, sizeof(struct open_work));
        task->extra = work;
        work->list = list;
        work->timer = OPEN_STEPS;
        task->step = screens_open_step;
    }
}

static void screens_create(const struct screen_entry *entry)
{
    struct screen_links links;

    while (entry->screen) {
        links.back = entry->links.back;
        links.next = entry->links.next;
        screen_creates[entry->screen](&links);
        entry++;
    }
}

static void screens_destroy(const struct screen_entry *entry)
{
    while (entry->screen) {
        screen_destroys[entry->screen]();
        entry++;
    }
}

static void advertise_task_init(struct task *task, struct screen_start *start)
{
    struct advertise_work *work = task->extra;

    textures_init();
    trial_reset();
    work->last_screen = SCREEN_NONE;
    work->choice = OVERLAY_BACK;
    work->result = OVERLAY_RUNNING;
    work->closed = 0;
    work->mode_hidden = 1;
    work->wheel_hidden = 1;
    work->list = screen_lists[start->entrance];
    work->screen = work->list->screen;
    work->shown = work->list->screen;
    screens_open(work->list);
    work->changing = 0;
    work->timer = 0;
    menu_create();
}

static void advertise_task_step(struct task *task)
{
    struct advertise_work *work = task->extra;

    if (work->changing) {
        work->timer = CHANGE_STEPS;
        work->changing = 0;
    }
    if (work->timer) {
        work->timer--;
        if (work->timer <= 0) {
            if (work->pending == SCREEN_NONE) {
                screen_set(SCREEN_CLOSE);
            } else {
                screen_set(work->pending);
            }
        }
    }
    switch (work->screen) {
    case SCREEN_CLOSE:
        if (work->closed == 1) {
            work->screen = SCREEN_CLOSED;
        }
        break;
    case SCREEN_CLOSED:
        /* @hack spelling: each case sets result through ADVERTISE_WORK(), whose
         * task is this one. SHC then loads advertise_task (MOV.L @R1,R4 at
         * 0x8c901604), and reads its extra in each case (MOV.L @(44,R4),R3 at
         * 0x8c90162a). through work, it stores through R14, and 77 of 117
         * words differ */
        switch (work->choice) {
        case OVERLAY_DONE:
            ADVERTISE_WORK()->result = OVERLAY_DONE;
            break;
        case MENU_TRIAL:
            ADVERTISE_WORK()->result = MENU_TRIAL;
            break;
        case MENU_ADVENTURE:
            ADVERTISE_WORK()->result = MENU_ADVENTURE;
            break;
        case MENU_INTERNET:
            ADVERTISE_WORK()->result = MENU_INTERNET;
            break;
        case MENU_STAFF_ROLL:
            ADVERTISE_WORK()->result = MENU_STAFF_ROLL;
            break;
        case MENU_TUTORIAL:
            ADVERTISE_WORK()->result = MENU_TUTORIAL;
            break;
        case MENU_UNK_106:
            ADVERTISE_WORK()->result = MENU_UNK_106;
            break;
        case MENU_DEMO:
            ADVERTISE_WORK()->result = MENU_DEMO;
            stage_set_entrance(0);
            break;
        default:
            ADVERTISE_WORK()->result = OVERLAY_BACK;
        }
        textures_release_all();
        advertise_apply();
        work->screen = SCREEN_DONE;
        break;
    default:
        work->closed = 0;
        break;
    }
}

static void advertise_task_destroy(struct task *task)
{
    struct advertise_work *work = task->extra;

    textures_release_all();
    screens_destroy(work->list);
    menu_kill();
}

struct task *screen_create(struct screen_start *start)
{
    struct task *task;
    struct advertise_work *work;

    if (advertise_task) {
        for (;;) {
        }
    }
    /* @hack spelling: the task assigned in the test. SHC then tests it in R0
     * (TST R0,R0 at 0x8c901706), and copies it to R14 after. assigned first,
     * it is copied to R14 and tested there, and 2 of 38 words differ */
    if ((advertise_task = task = task_create(0, 3, advertise_task_step))) {
        work = heap_calloc(1, sizeof(struct advertise_work));
        task->extra = work;
        advertise_task_init(task, start);
        task->step = advertise_task_step;
        task->destructor = advertise_task_destroy;
    }
    return task;
}

void screen_kill(void)
{
    if (advertise_task) {
        task_destroy(advertise_task);
        advertise_task = NULL;
    }
}

static void screen_set(int screen)
{
    if (ADVERTISE_WORK()->screen != screen) {
        ADVERTISE_WORK()->last_screen = ADVERTISE_WORK()->screen;
        ADVERTISE_WORK()->screen = screen;
    }
}

void screen_go_wheel(int next)
{
    character_select_set_next(next);
    screen_go(SCREEN_CHARACTER);
}

void screen_go(int screen)
{
    struct advertise_work *work = ADVERTISE_WORK();

    advertise_sound_stop();
    work->changing = 1;
    work->pending = screen;
}

/* the GAME BUG screen, which prints its name. mode select makes it the
 * wheel's next screen, but the wheel never goes on to its next. nothing else
 * opens it */
#include <ninja.h>
#include <sg_xpt.h>

#include "advertise/buttons.h"
#include "advertise/draw.h"
#include "advertise/game_bug.h"
#include "advertise/screen.h"
#include "advertise/textures.h"
#include "draw/screen.h"
#include "main.h"
#include "system/task.h"

#define CHOSEN_STEPS 10 /* the steps from a choice to going on */

enum game_bug_state { GAME_BUG_WAIT, GAME_BUG_CHOSEN, GAME_BUG_DONE };

struct game_bug {
    enum screen_mode mode;
    int back;
    int next;
    Float z;
    Float fade;
    Uint32 tint;
    enum game_bug_state state;
    Sint16 timer;
};

static struct task *game_bug_task;

static void game_bug_open(struct game_bug *work)
{
    work->state = GAME_BUG_WAIT;
}

static void game_bug_run(struct game_bug *work)
{
    switch (work->state) {
    case GAME_BUG_WAIT:
        if (pad_choose_pressed() && textures_ready == TRUE) {
            work->state = GAME_BUG_CHOSEN;
        }
        if (pad_back_pressed()) {
            screen_go(work->back);
            work->state = GAME_BUG_DONE;
        }
        work->timer = 0;
        break;
    case GAME_BUG_CHOSEN:
        work->timer++;
        if (work->timer >= CHOSEN_STEPS) {
            screen_go(work->next);
            work->state = GAME_BUG_DONE;
        }
        break;
    case GAME_BUG_DONE:
        break;
    }
}

static void game_bug_init(struct task *task, struct screen_links *links)
{
    struct game_bug *work = task->extra;

    work->back = links->back;
    work->next = links->next;
    work->z = 52000.0F;
    /* @hack spelling: copy, a copy of work. SHC then holds work in R1 and 0
     * in R4 (MOV.L @(44,R4),R1 at 0x8c903f0e, MOV #0,R4 at 0x8c903f12).
     * without it, work takes R4 and 0 takes R5, and 8 of 13 words differ */
    {
        struct game_bug *copy = work;

        copy->mode = SCREEN_HIDDEN;
        copy->state = GAME_BUG_WAIT;
    }
}

static void game_bug_step(struct task *task)
{
    struct game_bug *work = task->extra;

    if (ADVERTISE_WORK()->screen == SCREEN_GAME_BUG &&
        work->mode == SCREEN_HIDDEN) {
        textures_load_screen(SCREEN_GAME_BUG);
        game_bug_open(work);
        work->fade = 0.0F;
        work->mode = SCREEN_OPENING;
    }
    if (ADVERTISE_WORK()->last_screen == SCREEN_GAME_BUG &&
        work->mode == SCREEN_SHOWN) {
        ADVERTISE_WORK()->choice = MENU_ADVENTURE;
        work->fade = 0.0F;
        work->mode = SCREEN_CLOSING;
    }
    switch (work->mode) {
    case SCREEN_HIDDEN:
        work->tint = 0;
        break;
    case SCREEN_OPENING:
        work->tint = color_fade_in(work->fade);
        work->fade += 0.1F;
        if (work->fade >= 1.0F) {
            work->mode = SCREEN_SHOWN;
        }
        break;
    case SCREEN_SHOWN:
        work->tint = 0xffffffff;
        ADVERTISE_WORK()->shown = SCREEN_GAME_BUG;
        game_bug_run(work);
        break;
    case SCREEN_CLOSING:
        work->tint = color_fade_out(work->fade);
        work->fade += 0.1F;
        if (work->fade >= 1.0F) {
            textures_switch(SCREEN_GAME_BUG);
            ADVERTISE_WORK()->closed = 1;
            work->mode = SCREEN_HIDDEN;
        }
        break;
    }
}

static void game_bug_draw(struct task *task)
{
    struct game_bug *work = task->extra;

    if (work->tint) {
        screen_blend_alpha();
        /* @hack spelling: the tint and the print as one expression. SHC then
         * calls njPrintC and returns (JSR at 0x8c904056). as two statements,
         * the print becomes a tail jump, the function runs 2 bytes longer, the
         * functions after it move, and 8 of 20 words differ */
        screen_quad_tint(work->tint), njPrintC(NJM_LOCATION(12, 9), "GAME BUG");
    }
}

struct task *game_bug_create(struct screen_links *links)
{
    struct task *task;
    struct game_bug *work;

    /* @hack spelling: the task assigned in the test. SHC then tests it in R0
     * (TST R0,R0 at 0x8c904074), and copies it to R14 after. assigned first,
     * it is copied to R14 and tested there, and 2 of 32 words differ */
    if ((game_bug_task = task = task_create(0, 3, game_bug_step)) != NULL) {
        work = heap_calloc(1, sizeof(struct game_bug));
        task->extra = work;
        game_bug_init(task, links);
        task->step = game_bug_step;
        task->draw = game_bug_draw;
    }
    return task;
}

void game_bug_destroy(void)
{
    if (game_bug_task) {
        task_destroy(game_bug_task);
    }
}

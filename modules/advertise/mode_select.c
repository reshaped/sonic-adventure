/* mode select, the first screen with a menu, over the game's title. its items
 * are adventure, trial, options and internet */
#include <ninja.h>
#include <sg_xpt.h>
#include <sofdec.h>

#include "advertise/card_screen.h"
#include "advertise/draw.h"
#include "advertise/menus.h"
#include "advertise/mode_select.h"
#include "advertise/screen.h"
#include "advertise/sound.h"
#include "advertise/textures.h"
#include "advertise/trial.h"
#include "draw/screen.h"
#include "hud/menu.h"
#include "main.h"
#include "object/common.h"
#include "sound/stream.h"
#include "system/task.h"

/* MODE_MENU's items, and TRIAL_MENU's */
enum mode_item {
    ITEM_ADVENTURE,
    ITEM_TRIAL,
    ITEM_OPTIONS,
    ITEM_INTERNET,
    ITEM_BACK /* MODE_MENU's cancel, past its four items */
};
enum trial_item { ITEM_ACTION, ITEM_MINI_GAME };

/* once the title's music has ended, IDLE_STEPS steps with no move of the
 * cursor close the menu, and DEMO_STEPS later the demo starts */
#define IDLE_STEPS 480
#define DEMO_STEPS 20

enum mode_state {
    MODE_SHOW,
    MODE_CHOOSING,
    MODE_ASK_TRIAL,
    MODE_IDLE, /* closing the menu for the demo */
    MODE_DEMO, /* waiting for the demo */
    MODE_REOPEN,
    MODE_DONE
};

struct mode_select {
    enum screen_mode mode;
    int back;
    int next;
    Float z;
    Float fade;
    Uint32 tint;
    enum mode_state state;
    /* the item chosen, or -1 while MODE_MENU waits. MODE_MENU opens again
     * with the cursor on it */
    int item;
    Sint16 timer;
    Sint8 unk_22;
};

/* MODE_MENU's labels, as menus.c's mode_items holds them. nothing reads this
 * copy */
/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const Uint8 mode_item_textures[] = {
    TITLE_ADVENTURE,
    TITLE_TRIAL,
    TITLE_OPTIONS,
    TITLE_INTERNET,
};
#ifdef GCC
/* keeps the value the tag in mode_select_run names: ITEM_BACK's row, the
 * fifth, holds MENU_MODE_ADVENTURE */
static const int mode_item_modes[] = {
    MENU_MODE_ADVENTURE, MENU_MODE_TRIAL,     MENU_MODE_OPTIONS,
    MENU_MODE_INTERNET,  MENU_MODE_ADVENTURE,
};
#else
static const int mode_item_modes[] = {
    MENU_MODE_ADVENTURE,
    MENU_MODE_TRIAL,
    MENU_MODE_OPTIONS,
    MENU_MODE_INTERNET,
};
#endif

static struct task *mode_select_task;

void mode_menu_draw(struct menu_view *view)
{
    njSetTexture(&ava_title_texlist);
    screen_quad_color(0xffffffff);
    screen_quad_draw_scaled(QUAD_CENTERED | TITLE_WINDOW,
                            view->x + (10.0F * view->scale_x),
                            view->y + (10.0F * view->scale_y), view->z + 0.3F,
                            view->scale_x * 3.969F, view->scale_y * 3.469F);
}

void trial_menu_draw(struct menu_view *view)
{
    njSetTexture(&ava_title_texlist);
    screen_quad_color(0xffffffff);
    screen_quad_draw_scaled(QUAD_CENTERED | TITLE_WINDOW,
                            view->x + (10.0F * view->scale_x),
                            view->y + (10.0F * view->scale_y), view->z + 0.3F,
                            view->scale_x * 3.969F, view->scale_y * 2.781F);
}

static void mode_select_open(struct mode_select *work)
{
    textures_load_screen(SCREEN_MODE);
    advertise_music_play(TUNE_TITLE);
    /* @hack spelling: copy, a copy of work. SHC then keeps work on the
     * stack (MOV.L R4,@R15 at 0x8c908c04) and loads it for the two stores
     * (MOV.L @R15,R4 at 0x8c908c10). without it, SHC keeps work in R14, the
     * function runs 4 bytes short, the functions after it move, and 25 of 32
     * words differ */
    {
        struct mode_select *copy = work;

        copy->item = ITEM_ADVENTURE;
        copy->state = MODE_SHOW;
    }
    /* @hack spelling: mode_visits through a pointer to menu_state. SHC then
     * pools &menu_state, and reads the field at a displacement (MOV.B
     * @(12,R4),R0 at 0x8c908c1c). through menu_state, the pool holds the
     * field's address, the function runs 2 bytes longer, the functions after it
     * move, and 19 of 32 words differ */
    {
        struct menu_state *state = &menu_state;

        state->mode_visits++;
    }
    card_screen_next_mode();
    /* @hack spelling: back set through mode_select_task, whose extra is work.
     * SHC then reads mode_select_task and stores through its extra (MOV.L
     * @(44,R2),R1 at 0x8c908c2a). through work, SHC keeps work in R14, the
     * function runs 8 bytes short, the functions after it move, and 23 of 32
     * words differ */
    ((struct mode_select *)mode_select_task->extra)->back = SCREEN_CARD;
    work->timer = 0;
}

static void mode_select_run(struct mode_select *work)
{
    Sint8 disabled[4];
    Sint8 trial_disabled[4];
    Sint8 count = 0;
    int result;

    if (trial_stage_total() + trial_mini_game_total() == 0) {
        disabled[count++] = ITEM_TRIAL;
    }
    if (!internet_open()) {
        disabled[count++] = ITEM_INTERNET;
    }
    disabled[count] = -1;
    switch (work->state) {
    case MODE_SHOW:
        advertise_menu_show(MODE_MENU, work->item, disabled);
        work->state = MODE_CHOOSING;
        /* fall through */
    case MODE_CHOOSING:
        /* @hack type: the result cast to Sint8 before item takes it, which
         * SHC sign-extends from its low byte (EXTS.B at 0x8c908d52); without
         * the cast, 84 of 277 words differ */
        /* NOLINTBEGIN(bugprone-signed-char-misuse) */
        if ((work->item = (Sint8)menu_result()) >= 0 &&
            textures_ready == TRUE) {
            /* NOLINTEND(bugprone-signed-char-misuse) */
            /* @bug memory: a choice of ITEM_BACK reads
             * mode_item_modes[4], past the table's four items (MOV.L
             * @(R0,R1),R3 at 0x8c908d72). the image holds 0 there, at
             * 0x8c90df24, so backing out sets menu_state.mode to
             * MENU_MODE_ADVENTURE */
            menu_state.mode = mode_item_modes[work->item];
            switch (work->item) {
            case ITEM_ADVENTURE:
                screen_go_wheel(SCREEN_GAME_BUG);
                work->state = MODE_DONE;
                break;
            case ITEM_TRIAL:
                count = 0;
                if (!trial_stage_total()) {
                    trial_disabled[count++] = ITEM_ACTION;
                }
                if (!trial_mini_game_total()) {
                    trial_disabled[count++] = ITEM_MINI_GAME;
                }
                trial_disabled[count] = -1;
                /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
                advertise_menu_popup(TRIAL_MENU, 0, trial_disabled);
                work->state = MODE_ASK_TRIAL;
                break;
            case ITEM_OPTIONS:
                screen_go(SCREEN_OPTIONS);
                work->state = MODE_DONE;
                break;
            case ITEM_INTERNET:
                ADVERTISE_WORK()->choice = MENU_INTERNET;
                screen_go(SCREEN_NONE);
                work->state = MODE_DONE;
                break;
            case ITEM_BACK:
                screen_go(work->back);
                work->state = MODE_DONE;
                break;
            }
        } else {
            if (advertise_music_state() == ADXT_STAT_PLAYEND) {
                if (menu_get_idle() >= IDLE_STEPS) {
                    work->state = MODE_IDLE;
                }
            } else {
                menu_set_idle(0);
            }
        }
        break;
    case MODE_ASK_TRIAL:
        /* @hack type: the result cast to Sint8 in the test and the switch,
         * which SHC sign-extends from its low byte (EXTS.B at 0x8c908e72 and
         * 0x8c908e78); without the casts, 17 of 277 words differ */
        if ((Sint8)(result = menu_result()) >= 0) {
            switch ((Sint8)result) {
            case ITEM_ACTION:
                screen_go_wheel(SCREEN_TRIAL_STAGE);
                menu_state.trial[0] = MENU_TRIAL_STAGE;
                work->state = MODE_DONE;
                break;
            case ITEM_MINI_GAME:
                screen_go_wheel(SCREEN_TRIAL_MINI_GAME);
                menu_state.trial[0] = MENU_TRIAL_MINI_GAME;
                work->state = MODE_DONE;
                break;
            default:
                work->state = MODE_REOPEN;
                break;
            }
        } else {
            if (advertise_music_state() == ADXT_STAT_PLAYEND) {
                if (menu_get_idle() >= IDLE_STEPS) {
                    work->state = MODE_IDLE;
                }
            } else {
                menu_set_idle(0);
            }
        }
        break;
    case MODE_IDLE:
        if (menu_close()) {
            work->timer = 0;
            work->state = MODE_DEMO;
        }
        break;
    case MODE_DEMO:
        if (work->timer++ >= DEMO_STEPS) {
            ADVERTISE_WORK()->choice = MENU_DEMO;
            screen_go(SCREEN_NONE);
            work->state = MODE_DONE;
        }
        break;
    case MODE_REOPEN:
        /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
        advertise_menu_popup(MODE_MENU, work->item, disabled);
        work->state = MODE_CHOOSING;
        break;
    case MODE_DONE:
        break;
    }
}

static void mode_select_draw_title(Float z)
{
    njSetTexture(&ava_title_texlist);
    screen_quad_color(0xffffffff);
    screen_quad_draw(TITLE_PIECE_1, 0.0F, 0.0F, z - 4.0F);
    screen_quad_draw(TITLE_PIECE_2, 0.0F, 256.0F, z - 4.0F);
    screen_quad_draw(TITLE_PIECE_3, 256.0F, 0.0F, z - 4.0F);
    screen_quad_draw(TITLE_PIECE_4, 256.0F, 256.0F, z - 4.0F);
    screen_quad_draw(TITLE_PIECE_5, 512.0F, 0.0F, z - 4.0F);
    screen_quad_draw(TITLE_PIECE_6, 512.0F, 128.0F, z - 4.0F);
    screen_quad_draw(TITLE_PIECE_7, 512.0F, 256.0F, z - 4.0F);
    screen_quad_draw(TITLE_PIECE_8, 512.0F, 384.0F, z - 4.0F);
}

static void mode_select_init(struct task *task, struct screen_links *links)
{
    struct mode_select *work = task->extra;

    work->back = links->back;
    work->next = links->next;
    work->z = 53000.0F;
    /* @hack spelling: copy, a copy of work. SHC then holds work in R1 and 0
     * in R4 (MOV.L @(44,R4),R1 at 0x8c90901a, MOV #0,R4 at 0x8c90901e).
     * without it, work takes R4 and 0 takes R5, and 10 of 16 words differ */
    {
        struct mode_select *copy = work;

        copy->mode = SCREEN_HIDDEN;
        copy->unk_22 = 0;
        copy->item = ITEM_ADVENTURE;
        copy->state = MODE_SHOW;
    }
}

static void mode_select_step(struct task *task)
{
    struct mode_select *work = task->extra;

    if (ADVERTISE_WORK()->screen == SCREEN_MODE &&
        work->mode == SCREEN_HIDDEN) {
        mode_select_open(work);
        work->fade = 0.0F;
        work->mode = SCREEN_OPENING;
    }
    if (ADVERTISE_WORK()->last_screen == SCREEN_MODE &&
        work->mode == SCREEN_SHOWN) {
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
        work->unk_22 = 0;
        ADVERTISE_WORK()->mode_hidden = 0;
        work->tint = 0xffffffff;
        ADVERTISE_WORK()->shown = SCREEN_MODE;
        mode_select_run(work);
        break;
    case SCREEN_CLOSING:
        work->tint = color_fade_out(work->fade);
        work->fade += 0.1F;
        if (work->fade >= 1.0F) {
            ADVERTISE_WORK()->mode_hidden = 1;
            textures_switch(SCREEN_MODE);
            ADVERTISE_WORK()->closed = 1;
            work->mode = SCREEN_HIDDEN;
        }
        break;
    }
}

static void mode_select_draw(struct task *task)
{
    struct mode_select *work = task->extra;

    if (work->tint) {
        /* @hack spelling: the three calls as one expression. SHC then calls the
         * title's draw and returns (BSR at 0x8c90918a). as three statements,
         * the draw becomes a tail branch, the function runs 4 bytes longer, the
         * functions after it move, and 6 of 18 words differ */
        screen_blend_alpha(), screen_quad_tint(work->tint),
            mode_select_draw_title(work->z);
    }
}

struct task *mode_select_create(struct screen_links *links)
{
    struct task *task;
    struct mode_select *work;

    /* @hack spelling: the task assigned in the test. SHC then tests it in R0
     * (TST R0,R0 at 0x8c9091a8), and copies it to R14 after. assigned first,
     * it is copied to R14 and tested there, and 2 of 32 words differ */
    if ((mode_select_task = task = task_create(0, 4, mode_select_step))) {
        work = heap_calloc(1, sizeof(struct mode_select));
        task->extra = work;
        mode_select_init(task, links);
        task->step = mode_select_step;
        task->draw = mode_select_draw;
    }
    return task;
}

void mode_select_destroy(void)
{
    if (mode_select_task) {
        task_destroy(mode_select_task);
    }
}

void mode_select_back_card(void)
{
    ((struct mode_select *)mode_select_task->extra)->back = SCREEN_CARD;
}

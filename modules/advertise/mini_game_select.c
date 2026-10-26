/* the trial's mini game list, which picks one of the mini games the trial
 * can play for menu_state.story. it shows the emblems and best three records
 * of the one under the cursor */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "advertise/back.h"
#include "advertise/buttons.h"
#include "advertise/draw.h"
#include "advertise/emblems.h"
#include "advertise/menus.h"
#include "advertise/mini_game_select.h"
#include "advertise/records.h"
#include "advertise/screen.h"
#include "advertise/sound.h"
#include "advertise/textures.h"
#include "advertise/trial.h"
#include "draw/screen.h"
#include "hud/menu.h"
#include "main.h"
#include "object/common.h"
#include "save/save.h"
#include "system/pad.h"
#include "system/task.h"

#define LIST_ROWS 5 /* rows shown at once */
/* the cursor's row among those shown, from 0 at the top, while the list can
 * scroll */
#define LIST_MARGIN 2

/* ava_stnam's name of the bosses, which share it, and of Ice Cap, whose mini
 * game is its board */
#define STNAM_BOSS 32
#define STNAM_ICE_CAP 6

/* MINI_GAME_MENU's items */
enum mini_game_item { ITEM_DECIDE, ITEM_CANCEL };

enum mini_game_state { LIST_CHOOSING, LIST_ASKING, LIST_DONE };

struct mini_game_select {
    enum screen_mode mode;
    int back;
    int next;
    Float z;
    Float fade;
    Uint32 tint;
    enum mini_game_state state;
    /* the array list.stages points to, with room for the longest trial
     * list, though the longest mini game list has 6 entries */
    struct trial_stage room[LONGEST_TRIAL_LIST];
    struct trial_stages list;
    /* each row's name, as an index into stage_name_textures. it has room for
     * a row for each name */
    int names[STAGE_NAMES];
    Uint8 count;
    Uint32 cursor_color;
    Float pulse;
    Angle blink;
    Uint8 alpha;
    Uint8 scroll; /* the first row shown */
    Uint8 cursor;
};

static const Uint8 rank_textures[] = {ACTSEL_1ST, ACTSEL_2ND, ACTSEL_3RD};

static struct task *mini_game_select_task;

static void mini_game_select_draw_panel(struct mini_game_select *work);

void mini_game_menu_draw(struct menu_view *view)
{
    struct mini_game_select *work = mini_game_select_task->extra;

    njSetTexture(&ava_dlg_texlist);
    screen_quad_color(0xffffffff);
    screen_quad_draw_scaled(QUAD_CENTERED | DLG_SS_PLAYER,
                            (-218.0F * view->scale_x) + view->x,
                            (-19.0F * view->scale_y) + view->y, view->z - 6.0F,
                            view->scale_x, view->scale_y);
    screen_quad_draw_scaled(QUAD_CENTERED | DLG_SS_STAGE,
                            (-218.0F * view->scale_x) + view->x,
                            (19.0F * view->scale_y) + view->y, view->z - 6.0F,
                            view->scale_x, view->scale_y);
    njSetTexture(&ava_stnam_texlist);
    screen_quad_draw_scaled(
        stage_name_textures[work->names[work->cursor]] | QUAD_CENTERED,
        (-70.0F * view->scale_x) + view->x, (19.0F * view->scale_y) + view->y,
        view->z - 8.0F, view->scale_x, view->scale_y);
    tx_chnam_draw(menu_state.story, 0xffffffff,
                  (-70.0F * view->scale_x) + view->x,
                  (-19.0F * view->scale_y) + view->y, view->z - 8.0F,
                  view->scale_x, view->scale_y);
    menu_draw_column_item(0xff12b4ff, (-70.0F * view->scale_x) + view->x,
                          (-19.0F * view->scale_y) + view->y, view->z - 6.0F,
                          0.695F * view->scale_x, 0.53F * view->scale_y);
    menu_draw_column_item(0xff12b4ff, (-70.0F * view->scale_x) + view->x,
                          (19.0F * view->scale_y) + view->y, view->z - 6.0F,
                          0.695F * view->scale_x, 0.53F * view->scale_y);
}

/* TRUE unless the mini game under the cursor is a boss or Ice Cap's board,
 * which have no emblems to win. it tells them by their names */
static Bool mini_game_has_emblems(struct mini_game_select *work)
{
    if (stage_name_textures[work->names[work->cursor]] == STNAM_BOSS ||
        stage_name_textures[work->names[work->cursor]] == STNAM_ICE_CAP) {
        return FALSE;
    }
    return TRUE;
}

static void mini_game_select_name(struct mini_game_select *work)
{
    Uint8 i;

    work->count = 0;
    for (i = 0; i < work->list.count; i++) {
        work->names[work->count] =
            trial_stage_name_numbers[work->list.stages[i].stage];
        work->count++;
    }
}

static void mini_game_select_reset(struct mini_game_select *work)
{
    trial_mini_game_list(&work->list);
    mini_game_select_name(work);
    work->blink = 0;
    work->pulse = 1.0F;
    work->alpha = 255;
    work->state = LIST_CHOOSING;
}

static void mini_game_select_open(struct mini_game_select *work)
{
    textures_load_screen(SCREEN_TRIAL_MINI_GAME);
    advertise_music_play(TUNE_TRIAL);
    advertise_voice_summary(LINE_MINI_GAME);
    menu_state.trial[0] = MENU_TRIAL_MINI_GAME;
    mini_game_select_reset(work);
    work->scroll = 0;
    work->cursor = 0;
}

static void mini_game_select_choose(struct mini_game_select *work)
{
    trial_choice = work->list.stages[work->cursor];
    menu_state.stage = work->list.stages[work->cursor].stage;
    menu_state.act = work->list.stages[work->cursor].act;
    ADVERTISE_WORK()->choice = MENU_TRIAL;
}

static void mini_game_select_run(struct mini_game_select *work)
{
    int result;

    menu_cursor_step(&work->alpha, &work->blink, &work->pulse);
    work->cursor_color = 0xffff0000;
    switch (work->state) {
    case LIST_CHOOSING:
        work->cursor_color = menu_blend_color(
            0xff12b4ff, 0xffff0000, (njCos(work->blink) + 1.0F) * 0.5F);
        if (pad_press_repeat[0] & PDD_DGT_KU) {
            if (work->cursor > 0) {
                work->pulse = 1.5F;
                work->cursor--;
                if (work->scroll > 0 &&
                    work->cursor < work->scroll + LIST_MARGIN) {
                    work->scroll--;
                }
                se_move();
            }
        } else if (pad_press_repeat[0] & PDD_DGT_KD) {
            if (work->cursor < work->count - 1) {
                work->pulse = 1.5F;
                work->cursor++;
                if (work->cursor > work->scroll + LIST_MARGIN &&
                    work->scroll + LIST_ROWS < work->count) {
                    work->scroll++;
                }
                se_move();
            }
        } else if (pad_choose_pressed()) {
            advertise_menu_popup_default(MINI_GAME_MENU);
            work->state = LIST_ASKING;
        } else if (pad_back_pressed()) {
            if (textures_ready == TRUE) {
                screen_go(work->back);
                se_cancel();
                work->state = LIST_DONE;
            }
        }
        break;
    case LIST_ASKING:
        /* @hack type: the result cast to Sint8 in the test and the switch,
         * which SHC sign-extends from its low byte (EXTS.B at 0x8c9059e2 and
         * 0x8c9059e8); without the casts, 32 of 182 words differ */
        if ((Sint8)(result = menu_result()) >= 0) {
            switch ((Sint8)result) {
            case ITEM_DECIDE:
                screen_go(work->next);
                work->state = LIST_DONE;
                break;
            case ITEM_CANCEL:
                mini_game_select_reset(work);
                break;
            }
        }
        break;
    case LIST_DONE:
        break;
    }
}

static void mini_game_select_draw_title(struct mini_game_select *work)
{
    screen_quad_color(0xffffffff);
    njSetTexture(&ava_trialactsel_texlist);
    screen_quad_draw(ACTSEL_MINI_GAME, 32.0F, 42.0F, work->z - 14.0F);
    screen_band_draw(0.0F, 32.0F, work->z - 8.0F, 565.0F, 42.0F);
    mini_game_select_draw_panel(work);
}

static void mini_game_select_draw_panel(struct mini_game_select *work)
{
    int i;
    Float y;

    screen_quad_colors(0x78117bff, 0x78002e67, 0x78117bff, 0x78002e67);
    screen_box_draw(256.0F, 152.0F, work->z - 4.0F, 352.0F, 276.0F);
    njSetTexture(&ava_trialactsel_texlist);
    screen_quad_color(0xffffffff);
    if (mini_game_has_emblems(work)) {
        screen_quad_draw(QUAD_CENTERED | ACTSEL_EMBLEMS, 355.0F, 202.0F,
                         work->z - 8.0F);
    }
    for (i = 0; i < 4; i++) {
        y = 168.0F + (65.0F * i);
        if (i > 0 || mini_game_has_emblems(work)) {
            act_panel_draw(266.0F, y, work->z - 6.0F,
                           i == 0 ? PANEL_EMBLEMS_2 : PANEL_RECORD);
        }
        if (i > 0) {
            njSetTexture(&ava_trialactsel_texlist);
            screen_quad_color(0xffffffff);
            screen_quad_draw(rank_textures[i - 1], 358.0F, y + 12.0F,
                             work->z - 8.0F);
        }
    }
}

static void mini_game_select_draw_emblems(struct mini_game_select *work)
{
    int i;

    if (mini_game_has_emblems(work)) {
        for (i = 0; i <= 1; i++) {
            if (emblem_won(menu_state.story,
                           work->list.stages[work->cursor].stage, i)) {
                ava_emblem_draw(480.0F + (77.0F * i), 194.0F, work->z - 12.0F);
            }
        }
    }
}

static void mini_game_select_draw_records(struct mini_game_select *work)
{
    /* @hack type: the stage cast to Sint8. SHC then passes the byte to
     * record_kind as MOV.B loads it (0x8c905e2e). as the Uint8, SHC adds an
     * EXTU.B, and 66 of 150 words differ */
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    int stage = (Sint8)work->list.stages[work->cursor].stage;
    enum record_kind kind = record_kind(menu_state.story, stage);
    int i;
    int story;
    Uint8 label;
    Float y;
    struct save_time time;

    for (i = 0; i < 3; i++) {
        y = 260.0F + (65.0F * i);
        story = menu_state.story;
        if (kind == RECORD_SCORE) {
            new16no_draw(record_score(story, stage, i), 6, 0, 463.0F, y,
                         work->z - 12.0F, 1.0F, 1.0F);
            label = ACTSEL_SCORE;
        } else {
            if (kind == RECORD_CART_TIME) {
                record_cart_time(&time, story, stage, i);
                time_draw_hundredths(&time, 458.0F, y, work->z - 12.0F);
            } else {
                record_trial_time(&time, story, stage, i);
                time_draw(&time, 458.0F, y, work->z - 12.0F);
            }
            label = ACTSEL_TIME;
        }
        njSetTexture(&ava_trialactsel_texlist);
        screen_quad_color(0xffffffff);
        screen_quad_draw(label | QUAD_CENTERED, 320.0F,
                         202.0F + (65.0F * (i + 1)), work->z - 14.0F);
    }
}

static void mini_game_select_draw_list(struct mini_game_select *work)
{
    int rows = work->count > LIST_ROWS ? LIST_ROWS : work->count;
    int i;
    Uint32 color;

    screen_quad_colors(0x78117bff, 0x78002e67, 0x78117bff, 0x78002e67);
    screen_box_draw(32.0F, 152.0F, work->z - 6.0F, 208.0F, 276.0F);
    for (i = 0; i < rows; i++) {
        screen_quad_color(0xffffffff);
        njSetTexture(&ava_stnam_texlist);
        screen_quad_draw(stage_name_textures[work->names[work->scroll + i]] |
                             QUAD_CENTERED,
                         136.0F, 194.0F + (48.0F * i), work->z - 12.0F);
        if (work->cursor == work->scroll + i) {
            /* @hack type: alpha passed as a Sint8. SHC then passes the byte
             * as MOV.B loads it, in the JSR's slot (0x8c9060cc). as the Uint8,
             * SHC adds an EXTU.B there, and 10 of 213 words differ */
            /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
            menu_draw_column_cursor((Sint8)work->alpha, 136.0F,
                                    194.0F + (48.0F * i), work->z - 14.0F,
                                    0.695F * work->pulse, 0.531F * work->pulse);
            color = work->cursor_color;
        } else {
            color = 0xff12b4ff;
        }
        menu_draw_column_item(color, 136.0F, 194.0F + (48.0F * i),
                              work->z - 8.0F, 0.695F, 0.531F);
    }
    if (work->scroll > 0) {
        scroll_arrow_draw(TRUE, 136.0F, 171.0F, work->z - 12.0F);
    }
    if (work->scroll + LIST_ROWS < work->count) {
        scroll_arrow_draw(FALSE, 136.0F, 408.0F, work->z - 12.0F);
    }
}

static void mini_game_select_init(struct task *task, struct screen_links *links)
{
    struct mini_game_select *work = task->extra;

    work->back = links->back;
    work->next = links->next;
    work->z = 51000.0F;
    work->mode = SCREEN_HIDDEN;
    work->list.stages = work->room;
    mini_game_select_reset(work);
}

static void mini_game_select_step(struct task *task)
{
    struct mini_game_select *work = task->extra;

    if (ADVERTISE_WORK()->screen == SCREEN_TRIAL_MINI_GAME &&
        work->mode == SCREEN_HIDDEN) {
        mini_game_select_open(work);
        work->fade = 0.0F;
        work->mode = SCREEN_OPENING;
    }
    if (ADVERTISE_WORK()->last_screen == SCREEN_TRIAL_MINI_GAME &&
        work->mode == SCREEN_SHOWN) {
        mini_game_select_choose(work);
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
        ADVERTISE_WORK()->shown = SCREEN_TRIAL_MINI_GAME;
        mini_game_select_run(work);
        break;
    case SCREEN_CLOSING:
        work->tint = color_fade_out(work->fade);
        work->fade += 0.1F;
        if (work->fade >= 1.0F) {
            textures_switch(SCREEN_TRIAL_MINI_GAME);
            ADVERTISE_WORK()->closed = 1;
            work->mode = SCREEN_HIDDEN;
        }
        break;
    }
}

static void mini_game_select_draw(struct task *task)
{
    struct mini_game_select *work = task->extra;

    if (work->tint != 0) {
        screen_blend_alpha();
        screen_quad_tint(work->tint);
        mini_game_select_draw_title(work);
        mini_game_select_draw_emblems(work);
        mini_game_select_draw_records(work);
        b_chnam_draw(menu_state.story, 0xffffffff, 158.0F, 90.0F,
                     work->z - 14.0F);
        mini_game_select_draw_list(work);
        back_draw(work->z - 2.0F);
    }
}

struct task *mini_game_select_create(struct screen_links *links)
{
    struct task *task;
    struct mini_game_select *work;

    /* @hack spelling: the task assigned in the test. SHC then tests it in R0
     * (TST R0,R0 at 0x8c906414), and copies it to R14 after. assigned first,
     * it is copied to R14 and tested there, and 2 of 32 words differ */
    if ((mini_game_select_task = task =
             task_create(0, 4, mini_game_select_step)) != NULL) {
        work = heap_calloc(1, sizeof(struct mini_game_select));
        task->extra = work;
        mini_game_select_init(task, links);
        task->step = mini_game_select_step;
        task->draw = mini_game_select_draw;
    }
    return task;
}

void mini_game_select_destroy(void)
{
    if (mini_game_select_task) {
        task_destroy(mini_game_select_task);
    }
}

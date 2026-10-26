/* the trial's action stage list, which picks one of the action stages the
 * trial can play for menu_state.story. it shows the emblems and records of the
 * one under the cursor */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "advertise/back.h"
#include "advertise/buttons.h"
#include "advertise/draw.h"
#include "advertise/emblems.h"
#include "advertise/menus.h"
#include "advertise/records.h"
#include "advertise/screen.h"
#include "advertise/sound.h"
#include "advertise/textures.h"
#include "advertise/trial.h"
#include "advertise/trial_stage_select.h"
#include "advertise/wheel.h"
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

/* the record panel's rows: the emblems, the best score, the best time or Big's
 * best weight, and the most rings */
#define PANEL_ROWS 4
/* ava_new16no's g, which follows a weight in grams */
#define NEW16NO_GRAMS 3

/* TRIAL_STAGE_MENU's items */
enum trial_stage_item { ITEM_DECIDE, ITEM_CANCEL };

/* the list moves its cursor, waits for TRIAL_STAGE_MENU's answer, or is done
 * once it has asked for another screen */
enum trial_stage_state { LIST_CHOOSING, LIST_ASKING, LIST_DONE };

struct trial_stage_select {
    enum screen_mode mode;
    int back;
    int next;
    Float z;
    Float fade;
    Uint32 tint;
    enum trial_stage_state state;
    /* the array list.stages points to, with room for Sonic's list, the
     * longest */
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

static const Uint8 row_textures[PANEL_ROWS] = {
    ACTSEL_EMBLEMS,
    ACTSEL_HISCORE,
    ACTSEL_BEST_CLEAR,
    ACTSEL_RINGS,
};
static const Uint8 big_row_textures[PANEL_ROWS] = {
    ACTSEL_EMBLEMS,
    ACTSEL_HISCORE,
    ACTSEL_BEST_WEIGHT,
    ACTSEL_RINGS,
};

static struct task *trial_stage_select_task;

static void trial_stage_select_draw_panel(struct trial_stage_select *work);

void trial_stage_menu_draw(struct menu_view *view)
{
    struct trial_stage_select *work = trial_stage_select_task->extra;

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

static void trial_stage_select_name(struct trial_stage_select *work)
{
    Uint8 i;

    work->count = 0;
    for (i = 0; i < work->list.count; i++) {
        work->names[work->count] =
            trial_stage_name_numbers[work->list.stages[i].stage];
        work->count++;
    }
}

/* list the stages again and go back to choosing, with the cursor's pulse at
 * rest. the cursor and the scroll stay where they are */
static void trial_stage_select_reset(struct trial_stage_select *work)
{
    trial_stage_list(&work->list);
    trial_stage_select_name(work);
    work->state = LIST_CHOOSING;
    work->blink = 0;
    work->pulse = 1.0F;
    work->alpha = 255;
}

static void trial_stage_select_open(struct trial_stage_select *work)
{
    textures_load_screen(SCREEN_TRIAL_STAGE);
    advertise_music_play(TUNE_TRIAL);
    advertise_voice_summary(LINE_TRIAL_STAGE);
    menu_state.trial[0] = MENU_TRIAL_STAGE;
    trial_stage_select_reset(work);
    work->scroll = 0;
    work->cursor = 0;
}

static void trial_stage_select_choose(struct trial_stage_select *work)
{
    trial_choice = work->list.stages[work->cursor];
    menu_state.stage = work->list.stages[work->cursor].stage;
    menu_state.act = work->list.stages[work->cursor].act;
    ADVERTISE_WORK()->choice = MENU_TRIAL;
}

static void trial_stage_select_run(struct trial_stage_select *work)
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
            advertise_menu_popup_default(TRIAL_STAGE_MENU);
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
         * which SHC sign-extends from its low byte (EXTS.B at 0x8c909782 and
         * 0x8c909788). without the casts, the function runs 2 bytes shorter,
         * the code after it moves, and 39 of 182 words differ */
        if ((Sint8)(result = menu_result()) >= 0) {
            switch ((Sint8)result) {
            case ITEM_DECIDE:
                screen_go(work->next);
                work->state = LIST_DONE;
                break;
            case ITEM_CANCEL:
                trial_stage_select_reset(work);
                break;
            }
        }
        break;
    case LIST_DONE:
        break;
    }
}

/* draw the title and its band, then the panel the records sit in */
static void trial_stage_select_draw_title(struct trial_stage_select *work)
{
    screen_quad_color(0xffffffff);
    njSetTexture(&ava_trialactsel_texlist);
    screen_quad_draw(ACTSEL_TITLE, 32.0F, 42.0F, work->z - 14.0F);
    screen_band_draw(0.0F, 32.0F, work->z - 8.0F, 565.0F, 42.0F);
    trial_stage_select_draw_panel(work);
}

static void trial_stage_select_draw_panel(struct trial_stage_select *work)
{
    int i;
    Float y;
    Uint8 texture;

    screen_quad_colors(0x78117bff, 0x78002e67, 0x78117bff, 0x78002e67);
    screen_box_draw(256.0F, 152.0F, work->z - 4.0F, 352.0F, 276.0F);
    njSetTexture(&ava_trialactsel_texlist);
    for (i = 0; i < PANEL_ROWS; i++) {
        y = 168.0F + (65.0F * i);
        act_panel_draw(266.0F, y, work->z - 6.0F,
                       i == 0 ? PANEL_EMBLEMS_3 : PANEL_RECORD);
        njSetTexture(&ava_trialactsel_texlist);
        screen_quad_color(0xffffffff);
        texture = menu_state.story == STORY_BIG ? big_row_textures[i]
                                                : row_textures[i];
        screen_quad_draw(texture | QUAD_CENTERED, 355.0F, y + 34.0F,
                         work->z - 8.0F);
    }
}

static void trial_stage_select_draw_emblems(struct trial_stage_select *work)
{
    int mission;

    for (mission = 0; mission <= 2; mission++) {
        if (emblem_won(menu_state.story, work->list.stages[work->cursor].stage,
                       mission)) {
            ava_emblem_draw(470.0F + (50.0F * mission), 194.0F,
                            work->z - 12.0F);
        }
    }
}

static void trial_stage_select_draw_score(struct trial_stage_select *work)
{
    /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
    int score = record_stage_score(menu_state.story,
                                   work->list.stages[work->cursor].stage);

    new16no_draw(score, 6, 0, 463.0F, 260.0F, work->z - 12.0F, 1.0F, 1.0F);
}

/* draw the best time, or for Big his best fishing score, as a weight in
 * grams */
static void trial_stage_select_draw_time(struct trial_stage_select *work)
{
    struct save_time time;
    int weight;

    if (menu_state.story != STORY_BIG) {
        /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
        record_stage_time(&time, menu_state.story,
                          work->list.stages[work->cursor].stage);
        time_draw(&time, 458.0F, 325.0F, work->z - 12.0F);
    } else {
        /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
        weight = record_fishing_score(work->list.stages[work->cursor].stage);
        new16no_draw(weight, 5, 0, 472.0F, 325.0F, work->z - 12.0F, 1.0F, 1.0F);
        njSetTexture(&ava_new16no_texlist);
        screen_quad_color(0xffffffff);
        screen_quad_draw(QUAD_CENTERED | NEW16NO_GRAMS, 564.0F, 325.0F,
                         work->z - 12.0F);
    }
}

static void trial_stage_select_draw_rings(struct trial_stage_select *work)
{
    /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
    Sint16 rings = record_stage_rings(menu_state.story,
                                      work->list.stages[work->cursor].stage);

    new16no_draw(rings, 6, 0, 463.0F, 390.0F, work->z - 12.0F, 1.0F, 1.0F);
}

static void trial_stage_select_draw_records(struct trial_stage_select *work)
{
    trial_stage_select_draw_emblems(work);
    trial_stage_select_draw_score(work);
    trial_stage_select_draw_time(work);
    trial_stage_select_draw_rings(work);
}

static void trial_stage_select_draw_list(struct trial_stage_select *work)
{
    int rows = work->count > LIST_ROWS ? LIST_ROWS : work->count;
    int i;
    Uint32 color;

    screen_quad_colors(0x78117bff, 0x78002e67, 0x78117bff, 0x78002e67);
    screen_box_draw(32.0F, 152.0F, work->z - 6.0F, 208.0F, 276.0F);
    for (i = 0; i < rows; i++) {
        njSetTexture(&ava_stnam_texlist);
        screen_quad_color(0xffffffff);
        screen_quad_draw(stage_name_textures[work->names[work->scroll + i]] |
                             QUAD_CENTERED,
                         136.0F, 194.0F + (48.0F * i), work->z - 12.0F);
        if (work->cursor == work->scroll + i) {
            /* @hack type: alpha passed as a Sint8. SHC then passes the byte
             * as MOV.B loads it, in the JSR's slot (0x8c909dcc). as the Uint8,
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

static void trial_stage_select_init(struct task *task,
                                    struct screen_links *links)
{
    struct trial_stage_select *work = task->extra;

    work->back = links->back;
    work->next = links->next;
    work->z = 58000.0F;
    work->mode = SCREEN_HIDDEN;
    work->list.stages = work->room;
    trial_stage_select_reset(work);
}

static void trial_stage_select_step(struct task *task)
{
    struct trial_stage_select *work = task->extra;

    if (ADVERTISE_WORK()->screen == SCREEN_TRIAL_STAGE &&
        work->mode == SCREEN_HIDDEN) {
        trial_stage_select_open(work);
        work->fade = 0.0F;
        work->mode = SCREEN_OPENING;
    }
    if (ADVERTISE_WORK()->last_screen == SCREEN_TRIAL_STAGE &&
        work->mode == SCREEN_SHOWN) {
        trial_stage_select_choose(work);
        work->fade = 0.0F;
        work->mode = SCREEN_CLOSING;
    }
    switch (work->mode) {
    case SCREEN_HIDDEN:
        work->fade = 0.0F;
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
        work->fade = 0.0F;
        work->tint = 0xffffffff;
        ADVERTISE_WORK()->shown = SCREEN_TRIAL_STAGE;
        trial_stage_select_run(work);
        break;
    case SCREEN_CLOSING:
        work->tint = color_fade_out(work->fade);
        work->fade += 0.1F;
        if (work->fade >= 1.0F) {
            textures_switch(SCREEN_TRIAL_STAGE);
            ADVERTISE_WORK()->closed = 1;
            work->mode = SCREEN_HIDDEN;
        }
        break;
    }
}

static void trial_stage_select_draw(struct task *task)
{
    struct trial_stage_select *work = task->extra;

    if (work->tint != 0) {
        screen_blend_alpha();
        screen_quad_tint(work->tint);
        trial_stage_select_draw_title(work);
        trial_stage_select_draw_records(work);
        b_chnam_draw(menu_state.story, 0xffffffff, 158.0F, 90.0F,
                     work->z - 14.0F);
        trial_stage_select_draw_list(work);
        back_draw(work->z - 2.0F);
    }
}

struct task *trial_stage_select_create(struct screen_links *links)
{
    struct task *task;
    struct trial_stage_select *work;

    /* @hack spelling: the task assigned in the test. SHC then tests it in R0
     * (TST R0,R0 at 0x8c90a114), and copies it to R14 after. assigned first,
     * it is copied to R14 and tested there, and 2 of 32 words differ */
    if ((trial_stage_select_task = task =
             task_create(0, 4, trial_stage_select_step)) != NULL) {
        work = heap_calloc(1, sizeof(struct trial_stage_select));
        task->extra = work;
        trial_stage_select_init(task, links);
        task->step = trial_stage_select_step;
        task->draw = trial_stage_select_draw;
    }
    return task;
}

void trial_stage_select_destroy(void)
{
    if (trial_stage_select_task) {
        task_destroy(trial_stage_select_task);
    }
}

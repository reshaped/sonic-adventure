/* the wheel of characters: the screen that picks the story to play, or in
 * the trial the character to list stages or mini games for */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "advertise/back.h"
#include "advertise/buttons.h"
#include "advertise/character_select.h"
#include "advertise/draw.h"
#include "advertise/menus.h"
#include "advertise/preview.h"
#include "advertise/screen.h"
#include "advertise/sound.h"
#include "advertise/story_progress.h"
#include "advertise/textures.h"
#include "advertise/trial.h"
#include "advertise/wheel.h"
#include "draw/screen.h"
#include "hud/menu.h"
#include "main.h"
#include "object/common.h"
#include "system/pad.h"
#include "system/task.h"

/* the steps the wheel waits after the menu's answer before it goes on.
 * after a trial's pick, it waits one step more */
#define CHOSEN_STEPS 300

/* ava_chsel's textures. the board under the circle holds the name, and its
 * ends differ in the adventure, where they hold the progress and the file */
enum {
    CHSEL_BOARD,
    CHSEL_BOARD_END,
    CHSEL_TRIAL_BOARD_END,
    CHSEL_STEM, /* joins the circle to the board */
    CHSEL_DISC,
    CHSEL_RING_LOWER,
    CHSEL_RING_UPPER,
    CHSEL_PLAYER_SELECT,
    CHSEL_PROGRESS,
    CHSEL_TITLE
};
/* ava_suuji's percent sign */
#define SUUJI_PERCENT 11

/* the wheel's states, in the order wheel_draw tests. from WHEEL_TRIAL_CHOSEN
 * on, a menu asks about the top story or the story is picked. wheel_draw then
 * draws the top story's name alone, enlarged */
enum wheel_state {
    WHEEL_STILL,
    WHEEL_TURN_LEFT,
    WHEEL_TURN_RIGHT,
    WHEEL_TRIAL_CHOSEN, /* a trial's pick, waiting before the trial's list */
    /* the menu that asks what to start: for a story, a cleared story, the
     * last story, or the last story cleared */
    WHEEL_ASK,
    WHEEL_ASK_CLEARED,
    WHEEL_ASK_LAST,
    WHEEL_ASK_LAST_CLEARED,
    WHEEL_ANSWERED,
    /* an answer other than CHOICE_BACK, waiting before the menus close */
    WHEEL_CHOSEN,
    WHEEL_DONE
};

enum wheel_choice {
    CHOICE_STORY,
    CHOICE_TUTORIAL,
    CHOICE_STAFF_ROLL,
    CHOICE_BACK
};

struct character_select {
    enum screen_mode mode;
    int back;
    int next;
    Float z;
    Float fade;
    Uint32 tint;
    Angle pulse; /* the top story's name swells with this angle's sine */
    enum wheel_state state;
    int story; /* the story at the top of the wheel */
    Uint8 unk_24;
    /* how far the wheel has turned from the top story. it runs from 0 to 1
     * toward the story before it, and from 0 to -1 toward the one after */
    Float turn;
    Sint16 timer;
    enum wheel_choice choice;
    Uint8 unk_34;
};

/* each story's name width, at full size */
static const Float name_widths[] = {
    143.0F, 122.0F, 227.0F, 102.0F, 89.0F, 209.0F, 230.0F,
};

static struct task *character_select_task;

/* TRUE when the wheel picks for the trial, not the adventure. SHC inlines
 * every call but the one after character_select_open's STORY_LAST test (BSR
 * at 0x8c9030de) */
static Bool trial_mode(void)
{
    return menu_state.mode != MENU_MODE_ADVENTURE;
}

/* TRUE when the wheel can pick story, and FALSE when it can't. the trial must
 * be able to play one of the story's action stages, or in the mini game trial
 * one of its mini games. in the adventure, the story's character must be
 * playable */
static Bool story_open(int story)
{
    if (trial_mode()) {
        if (menu_state.trial[0] != MENU_TRIAL_STAGE) {
            return trial_mini_game_count(story) != 0;
        } else {
            return trial_stage_count(story) != 0;
        }
    } else {
        return character_playable(story);
    }
}

static void wheel_stop(struct character_select *work)
{
    work->pulse = 0;
    work->turn = 0.0F;
    work->state = WHEEL_STILL;
}

static void character_select_open(struct character_select *work)
{
    textures_load_screen(SCREEN_CHARACTER);
    preview_create();
    if (trial_mode()) {
        advertise_music_play(TUNE_TRIAL);
    } else {
        advertise_music_play(TUNE_CHARACTER);
    }
    advertise_voice_summary(LINE_CHARACTER);
    wheel_stop(work);
    if (work->story == STORY_LAST && trial_mode()) {
        work->story = 0;
        menu_state.story = work->story;
    }
}

/* set the story at the top to menu_state.story, and record it in
 * menu_state.wheel_story, the story the wheel opens at */
static void wheel_keep(struct character_select *work)
{
    work->story = menu_state.story;
    menu_state.wheel_story = work->story;
    menu_state.story = work->story;
}

/* @hack spelling: menu_state.story declared under a name of its own, for
 * character_select_run. SHC then pools its address, and keeps 1 in R9 and 2
 * in R8 (0x8c903176, 0x8c90317a). through menu_state, &menu_state takes R9,
 * the function runs 32 bytes short, the functions after it move, and 402 of
 * 421 words differ */
extern Uint8 menu_story;
/* @hack spelling: menu_state.trial[0] declared under a name of its own, for
 * character_select_run. through menu_state, SHC keeps preview_kill's address
 * in R9 and 10 in R8 in place of 1 and 2, the function runs 4 bytes short,
 * the functions after it move, and 289 of 421 words differ */
extern Uint8 menu_trial;

static void character_select_run(struct character_select *work)
{
    int result;

    switch (work->state) {
    case WHEEL_STILL:
        work->pulse += NJM_DEG_ANG(8);
        if (pad_press_repeat[0] & PDD_DGT_KL) {
            work->state = WHEEL_TURN_LEFT;
            se_move();
        } else if (pad_press_repeat[0] & PDD_DGT_KR) {
            work->state = WHEEL_TURN_RIGHT;
            se_move();
        } else if (pad_choose_pressed() && textures_ready == TRUE) {
            if (story_open(work->story)) {
                work->timer = 0;
                if (trial_mode()) {
                    advertise_voice_play(work->story);
                    preview_set_chosen(1);
                    work->state = WHEEL_TRIAL_CHOSEN;
                } else if (work->story == STORY_LAST) {
                    if (story_cleared(work->story)) {
                        advertise_menu_popup_default(LAST_STORY_CLEARED_MENU);
                        work->state = WHEEL_ASK_LAST_CLEARED;
                    } else {
                        advertise_menu_popup_default(LAST_STORY_MENU);
                        work->state = WHEEL_ASK_LAST;
                    }
                } else {
                    if (story_cleared(work->story)) {
                        advertise_menu_popup_default(STORY_CLEARED_MENU);
                        work->state = WHEEL_ASK_CLEARED;
                    } else {
                        advertise_menu_popup_default(STORY_MENU);
                        work->state = WHEEL_ASK;
                    }
                }
            }
            /* @hack redundant: a break where the case's own break follows. SHC
             * then loads pad_back_pressed's address for the test below into
             * R3, not R1 (0x8c9032c0); without it, 12 of 421 words differ */
            break;
        } else if (pad_back_pressed() && textures_ready == TRUE) {
            screen_go(work->back);
            se_cancel();
            preview_kill();
            work->state = WHEEL_DONE;
        }
        break;
    case WHEEL_TURN_LEFT:
        work->turn += 0.05F;
        if (work->turn >= 1.0F) {
            work->story = (work->story + wheel_count() - 1) % wheel_count();
            menu_story = work->story;
            wheel_stop(work);
        }
        break;
    case WHEEL_TURN_RIGHT:
        work->turn -= 0.05F;
        if (work->turn <= -1.0F) {
            work->story = (work->story + 1) % wheel_count();
            menu_story = work->story;
            wheel_stop(work);
            /* @hack redundant: a break where the case's own break follows. SHC
             * then gives WHEEL_TRIAL_CHOSEN's case below R3 for the timer and
             * R2 for menu_trial (0x8c90337e, 0x8c903392), not the reverse;
             * without it, 15 of 421 words differ */
            break;
        }
        break;
    case WHEEL_TRIAL_CHOSEN:
        if (work->timer++ < CHOSEN_STEPS) {
            break;
        }
        /* @hack redundant: a select on 1.0F, which is always true. SHC folds
         * it away, but then keeps screen_go's address in R2 and
         * preview_kill's in R3 (0x8c9033e2, 0x8c9033e8). without it, the two
         * trade, and 6 of 421 words differ */
        /* NOLINTBEGIN(readability-avoid-nested-conditional-operator) */
        screen_go(menu_trial ? SCREEN_TRIAL_MINI_GAME
                             : (1.0F ? SCREEN_TRIAL_STAGE : 0));
        /* NOLINTEND(readability-avoid-nested-conditional-operator) */
        preview_kill();
        work->state = WHEEL_DONE;
        break;
    case WHEEL_ASK:
        /* @hack type: the result cast to Sint8 in the test and the switch,
         * which SHC sign-extends from its low byte (EXTS.B at 0x8c9033fa and
         * 0x8c903404); without the casts, 50 of 421 words differ */
        if ((Sint8)(result = menu_result()) >= 0) {
            switch ((Sint8)result) {
            case 0:
                work->choice = CHOICE_STORY;
                break;
            case 1:
                work->choice = CHOICE_TUTORIAL;
                break;
            case 2:
                work->choice = CHOICE_BACK;
                break;
            }
            work->state = WHEEL_ANSWERED;
        }
        break;
    case WHEEL_ASK_CLEARED:
        /* @hack type: the result cast to Sint8 in the test and the switch,
         * which SHC sign-extends from its low byte (EXTS.B at 0x8c90342a and
         * 0x8c903434); without the casts, 26 of 421 words differ */
        if ((Sint8)(result = menu_result()) >= 0) {
            switch ((Sint8)result) {
            case 0:
                work->choice = CHOICE_STORY;
                break;
            case 1:
                work->choice = CHOICE_TUTORIAL;
                break;
            case 2:
                work->choice = CHOICE_STAFF_ROLL;
                break;
            case 3:
                work->choice = CHOICE_BACK;
                break;
            }
            work->state = WHEEL_ANSWERED;
        }
        break;
    case WHEEL_ASK_LAST:
        /* @hack type: the result cast to Sint8 in the test and the switch,
         * which SHC sign-extends from its low byte (EXTS.B at 0x8c903486 and
         * 0x8c903490); without the casts, 73 of 421 words differ */
        if ((Sint8)(result = menu_result()) >= 0) {
            switch ((Sint8)result) {
            case 0:
                work->choice = CHOICE_STORY;
                break;
            case 1:
                work->choice = CHOICE_BACK;
                break;
            }
            work->state = WHEEL_ANSWERED;
        }
        break;
    case WHEEL_ASK_LAST_CLEARED:
        /* @hack type: the result cast to Sint8 in the test and the switch,
         * which SHC sign-extends from its low byte (EXTS.B at 0x8c9034ae and
         * 0x8c9034b8); without the casts, 53 of 421 words differ */
        if ((Sint8)(result = menu_result()) >= 0) {
            switch ((Sint8)result) {
            case 0:
                work->choice = CHOICE_STORY;
                break;
            case 1:
                work->choice = CHOICE_STAFF_ROLL;
                break;
            case 2:
                work->choice = CHOICE_BACK;
                break;
            }
            work->state = WHEEL_ANSWERED;
        }
        break;
    case WHEEL_ANSWERED:
        if (work->choice == CHOICE_BACK) {
            wheel_stop(work);
        } else {
            work->timer = 0;
            work->state = WHEEL_CHOSEN;
            advertise_voice_play(work->story);
            preview_set_chosen(1);
        }
        break;
    case WHEEL_CHOSEN:
        if (++work->timer >= CHOSEN_STEPS) {
            /* the choice is never CHOICE_BACK here: WHEEL_ANSWERED stops the
             * wheel on it */
            switch (work->choice) { /* NOLINT(clang-diagnostic-switch) */
            case CHOICE_STORY:
                ADVERTISE_WORK()->choice = MENU_ADVENTURE;
                break;
            case CHOICE_TUTORIAL:
                ADVERTISE_WORK()->choice = MENU_TUTORIAL;
                break;
            case CHOICE_STAFF_ROLL:
                ADVERTISE_WORK()->choice = MENU_STAFF_ROLL;
                break;
            }
            screen_go(SCREEN_NONE);
            preview_kill();
            work->state = WHEEL_DONE;
        }
        break;
    case WHEEL_DONE:
        break;
    }
}

/* draw the title, the circle, and the board the name sits on */
static void character_select_draw_frame(struct character_select *work)
{
    njSetTexture(&ava_chsel_texlist);
    screen_quad_color(0xffffffff);
    screen_quad_draw(CHSEL_TITLE, 26.0F, 41.0F, work->z - 14.0F);
    screen_band_draw(0.0F, 32.0F, work->z - 8.0F, 557.0F, 41.0F);
    screen_quad_draw(QUAD_CENTERED | CHSEL_PLAYER_SELECT, 316.0F, 244.0F,
                     work->z - 12.0F);
    screen_quad_colors(0x79ff7a00, 0x79ffff00, 0x79ff7a00, 0x79ffff00);
    screen_quad_draw_scaled(QUAD_CENTERED | CHSEL_DISC, 316.0F, 244.0F,
                            work->z - 8.0F, 1.172F, 1.172F);
    screen_quad_colors(0xffe37500, 0xfffec800, 0xffe37500, 0xfffec800);
    screen_quad_draw_scaled(QUAD_BOTTOM | CHSEL_RING_UPPER, 316.0F, 244.0F,
                            work->z - 10.0F, 1.109F, 1.109F);
    screen_quad_colors(0xfffec800, 0xffe47900, 0xfffec800, 0xffe47900);
    screen_quad_draw_scaled(QUAD_TOP | CHSEL_RING_LOWER, 316.0F, 244.0F,
                            work->z - 10.0F, 1.109F, 1.109F);
    screen_quad_colors(0xff0016ff, 0xff002eff, 0xff0016ff, 0xff002eff);
    screen_quad_draw(QUAD_TOP | CHSEL_STEM, 316.0F, 312.0F, work->z - 8.0F);
    screen_quad_colors(0xff002eff, 0xff00cbff, 0xff002eff, 0xff00cbff);
    screen_quad_draw(QUAD_TOP | CHSEL_BOARD, 316.0F, 386.0F, work->z - 8.0F);
}

static void character_select_draw_name(struct character_select *work)
{
    Uint32 color;

    if (story_open(work->story)) {
        color = 0xffffffff;
    } else {
        color = 0x80c0c0c0;
    }
    b_chnam_draw(work->story, color, 164.0F, 390.0F, work->z - 12.0F);
}

static void wheel_draw(struct character_select *work)
{
    int step;
    int i;
    int story;
    Angle ang;
    Float turn;
    Float x;
    Float y;
    Float scale;
    Uint32 color;
    Uint32 tint;

    step = (int)((360.0F / wheel_count()) * (65536.0F / 360.0F));
    for (i = 0; i < wheel_count(); i++) {
        turn = work->turn;
        if (turn < -1.0F) {
            turn = -1.0F;
        }
        if (turn > 1.0F) {
            turn = 1.0F;
        }
        story = (work->story + i) % wheel_count();
        ang = (Angle)((step * i) + (step * turn));
        x = 316.0F + (126.0F * njSin(ang));
        y = 244.0F + (126.0F * njCos(ang));
        scale = 0.615F;
        if (story != work->story) {
            color = 0xff0000ff;
        } else {
            color = menu_blend_color(0xffff0000, 0xff0000ff,
                                     _builtin_fabsf(work->turn));
            if (work->turn == 0.0F) {
                scale =
                    0.615F + ((njSin(work->pulse) + 1.0F) * (0.615F * 0.3F));
            }
            if (work->state >= WHEEL_TRIAL_CHOSEN) {
                scale = 0.615F * 1.8F;
            }
        }
        menu_draw_row_item(color, x, y, work->z - 12.0F, 0.75F, 0.75F);
        if (work->state == WHEEL_DONE) {
            continue;
        }
        if (work->state < WHEEL_TRIAL_CHOSEN || story == work->story) {
            x += njSin(ang) * name_widths[story] * 0.5F;
            if (story_open(story)) {
                tint = 0xffffffff;
            } else {
                tint = 0x80c0c0c0;
                scale = 0.615F;
            }
            /* @hack redundant: x + 0.0F, which SHC adds (FLDI0 FR1 at
             * 0x8c903986, FADD FR1,FR4 at 0x8c90399a); without it, the
             * function runs 4 bytes short, the functions after it move, and
             * 57 of 201 words differ */
            m_chnam_draw(story, tint, x + 0.0F, y - 28.0F, work->z - 14.0F,
                         scale, scale);
        }
    }
}

/* draw the board's ends beside the name, and in the adventure the story's
 * progress and the save's number */
static void character_select_draw_captions(struct character_select *work)
{
    njSetTexture(&ava_chsel_texlist);
    screen_quad_colors(0xff002eff, 0xff00cbff, 0xff002eff, 0xff00cbff);
    if (trial_mode()) {
        screen_quad_draw(QUAD_TOP_RIGHT | CHSEL_TRIAL_BOARD_END, 188.0F, 386.0F,
                         work->z - 8.0F);
        screen_quad_draw(QUAD_FLIP_U | QUAD_FLIP_V | CHSEL_TRIAL_BOARD_END,
                         444.0F, 386.0F, work->z - 8.0F);
    } else {
        screen_quad_draw(QUAD_TOP_RIGHT | CHSEL_BOARD_END, 188.0F, 386.0F,
                         work->z - 8.0F);
        screen_quad_draw(QUAD_FLIP_U | QUAD_FLIP_V | CHSEL_BOARD_END, 444.0F,
                         386.0F, work->z - 8.0F);
        suuji_draw(story_progress(work->story), 3, 0, 487.0F, 418.8F,
                   work->z - 14.0F, 0.7F);
        fscmn_draw(menu_state.save, 125.9F, 414.9F, work->z - 14.0F);
        menu_draw_row_item(0xffff0000, 504.0F, 418.0F, work->z - 12.0F, 0.781F,
                           0.781F);
        menu_draw_row_item(0xffff0000, 128.0F, 418.0F, work->z - 12.0F, 0.781F,
                           0.781F);
        njSetTexture(&ava_suuji_texlist);
        screen_quad_color(0xffffffff);
        screen_quad_draw(QUAD_CENTERED | SUUJI_PERCENT, 520.0F, 418.8F,
                         work->z - 14.0F);
        njSetTexture(&ava_chsel_texlist);
        screen_quad_draw(CHSEL_PROGRESS, 538.0F, 394.0F, work->z - 12.0F);
        njSetTexture(&ava_fscmn_texlist);
        screen_quad_draw(FSCMN_FILE, 36.0F, 423.0F, work->z - 12.0F);
    }
}

static void character_select_init(struct task *task, struct screen_links *links)
{
    struct character_select *work = task->extra;

    work->back = links->back;
    work->next = links->next;
    work->z = 60000.0F;
    work->mode = SCREEN_HIDDEN;
    work->story = menu_state.wheel_story;
    work->unk_24 = 0;
    work->unk_34 = 0;
    wheel_stop(work);
}

static void character_select_step(struct task *task)
{
    struct character_select *work = task->extra;

    if (ADVERTISE_WORK()->screen == SCREEN_CHARACTER &&
        work->mode == SCREEN_HIDDEN) {
        character_select_open(work);
        work->fade = 0.0F;
        work->mode = SCREEN_OPENING;
    }
    if (ADVERTISE_WORK()->last_screen == SCREEN_CHARACTER &&
        work->mode == SCREEN_SHOWN) {
        work->fade = 0.0F;
        work->mode = SCREEN_CLOSING;
    }
    wheel_keep(work);
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
        work->unk_34 = 0;
        ADVERTISE_WORK()->wheel_hidden = 0;
        work->tint = 0xffffffff;
        ADVERTISE_WORK()->shown = SCREEN_CHARACTER;
        character_select_run(work);
        break;
    case SCREEN_CLOSING:
        work->tint = color_fade_out(work->fade);
        work->fade += 0.1F;
        if (work->fade >= 1.0F) {
            ADVERTISE_WORK()->wheel_hidden = 1;
            textures_switch(SCREEN_CHARACTER);
            ADVERTISE_WORK()->closed = 1;
            work->mode = SCREEN_HIDDEN;
        }
        break;
    }
}

static void character_select_draw(struct task *task)
{
    struct character_select *work = task->extra;

    if (work->tint) {
        screen_blend_alpha();
        screen_quad_tint(work->tint);
        character_select_draw_frame(work);
        character_select_draw_name(work);
        wheel_draw(work);
        character_select_draw_captions(work);
        back_draw(work->z - 4.0F);
    }
}

struct task *character_select_create(struct screen_links *links)
{
    struct task *task;
    struct character_select *work;

    /* @hack spelling: the task assigned in the test. SHC then tests it in R0
     * (TST R0,R0 at 0x8c903db4), and copies it to R14 after. assigned first,
     * it is copied to R14 and tested there, and 2 of 32 words differ */
    if ((character_select_task = task =
             task_create(0, 3, character_select_step))) {
        work = heap_calloc(1, sizeof(struct character_select));
        task->extra = work;
        character_select_init(task, links);
        task->step = character_select_step;
        task->draw = character_select_draw;
    }
    return task;
}

void character_select_destroy(void)
{
    if (character_select_task) {
        task_destroy(character_select_task);
    }
}

void character_select_set_next(int next)
{
    ((struct character_select *)character_select_task->extra)->next = next;
}

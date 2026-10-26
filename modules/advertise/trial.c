/* the trial's lists, which stories, stages and choices the menus open, and
 * what the menus hand the game as they close */
#include <sg_xpt.h>

#include "advertise/records.h"
#include "advertise/trial.h"
#include "advertise/wheel.h"
#include "object/common.h"
#include "player/player.h"
#include "progress/progress.h"
#include "save/card.h"
#include "save/mail.h"
#include "save/save.h"
#include "stage/stage.h"
#include "system/bitmap.h"
#include "system/texture.h"

/* the literal's NUL falls outside the array */
/* NOLINTNEXTLINE(clang-diagnostic-unterminated-string-initialization) */
const char voice_save_data[16] = "Team Andromeda\r\n";

/* a trial list of every entry in stages, an array */
#define TRIAL_LIST(stages) {(stages), sizeof(stages) / sizeof((stages)[0])}

static const struct trial_stage trial_sonic_stages[] = {
    {STAGE_BEACH, 0},   {STAGE_WINDY, 0},    {STAGE_TWINKLE, 0},
    {STAGE_HIGHWAY, 0}, {STAGE_MOUNTAIN, 0}, {STAGE_SKY_DECK, 0},
    {STAGE_RUIN, 0},    {STAGE_ICECAP, 0},   {STAGE_CASINO, 0},
    {STAGE_FINAL, 0},
};
static const struct trial_stage trial_tails_stages[] = {
    {STAGE_WINDY, 2},  {STAGE_HIGHWAY, 0}, {STAGE_SKY_DECK, 0},
    {STAGE_ICECAP, 2}, {STAGE_CASINO, 1},
};
static const struct trial_stage trial_knuckles_stages[] = {
    {STAGE_HIGHWAY, 2}, {STAGE_MOUNTAIN, 2}, {STAGE_SKY_DECK, 2},
    {STAGE_RUIN, 1},    {STAGE_CASINO, 0},
};
static const struct trial_stage trial_amy_stages[] = {
    {STAGE_TWINKLE, 1},
    {STAGE_FINAL, 0},
    {STAGE_SHELTER, 0},
};
static const struct trial_stage trial_big_stages[] = {
    {STAGE_BEACH, 2},
    {STAGE_TWINKLE, 1},
    {STAGE_ICECAP, 3},
    {STAGE_SHELTER, 0},
};
static const struct trial_stage trial_e102_stages[] = {
    {STAGE_BEACH, 0}, {STAGE_WINDY, 0},   {STAGE_MOUNTAIN, 1},
    {STAGE_FINAL, 2}, {STAGE_SHELTER, 2},
};
static const struct trial_stage trial_sonic_mini_games[] = {
    {STAGE_SHOOTING1, 0}, {STAGE_SHOOTING2, 0}, {STAGE_ICECAP, 2},
    {STAGE_SUNABOARD, 0}, {STAGE_CART, 0},      {STAGE_EGGMOBILE3, 0},
};
static const struct trial_stage trial_tails_mini_games[] = {
    {STAGE_SHOOTING1, 0}, {STAGE_SHOOTING2, 0}, {STAGE_ICECAP, 2},
    {STAGE_SUNABOARD, 0}, {STAGE_CART, 0},      {STAGE_EGGMOBILE2, 0},
};
static const struct trial_stage trial_knuckles_mini_games[] = {
    {STAGE_CART, 0},
    {STAGE_CHAOS6, 1},
};
static const struct trial_stage trial_amy_mini_games[] = {
    {STAGE_PRACTICE, 0},
    {STAGE_CART, 0},
    {STAGE_EGGMANROBO, 0},
};
static const struct trial_stage trial_big_mini_games[] = {
    {STAGE_CART, 0},
    {STAGE_CHAOS6, 0},
};
static const struct trial_stage trial_e102_mini_games[] = {
    {STAGE_CART, 0},
    {STAGE_E101_R, 0},
};

const struct trial_stages trial_stage_lists[] = {
    TRIAL_LIST(trial_sonic_stages),    TRIAL_LIST(trial_tails_stages),
    TRIAL_LIST(trial_knuckles_stages), TRIAL_LIST(trial_amy_stages),
    TRIAL_LIST(trial_big_stages),      TRIAL_LIST(trial_e102_stages),
};
static const struct trial_stages trial_mini_game_lists[] = {
    TRIAL_LIST(trial_sonic_mini_games),    TRIAL_LIST(trial_tails_mini_games),
    TRIAL_LIST(trial_knuckles_mini_games), TRIAL_LIST(trial_amy_mini_games),
    TRIAL_LIST(trial_big_mini_games),      TRIAL_LIST(trial_e102_mini_games),
};

const Uint8 file_stage_name_numbers[STAGE_COUNT] = {
    17, 3,  4,  7,  8,  9,  10, 12, 6,  5,  13, 0, 11, 0, 0,
    19, 20, 21, 22, 33, 23, 24, 25, 30, 31, 34, 1, 1,  1, 2,
    2,  2,  2,  0,  0,  18, 14, 15, 16, 0,  0,  0, 0,
};
const Uint8 trial_stage_name_numbers[STAGE_COUNT] = {
    17, 3,  4,  7,  8,  9,  10, 12, 6,  5,  13, 0, 11, 0, 0,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 1, 1,  1, 2,
    2,  2,  17, 0,  0,  18, 14, 15, 16, 0,  0,  0, 0,
};

const Uint16 story_characters[STORIES] = {
    CHARACTER_SONIC, CHARACTER_TAILS, CHARACTER_KNUCKLES, CHARACTER_AMY,
    CHARACTER_BIG,   CHARACTER_E102,  CHARACTER_SONIC,
};
/* by enum wheel_story, the event flag each story sets as it ends */
static const int story_end_flags[STORIES] = {
    FLAG_CLEAR_SONIC, FLAG_CLEAR_TAILS, FLAG_CLEAR_KNUCKLES, FLAG_CLEAR_AMY,
    FLAG_CLEAR_BIG,   FLAG_CLEAR_E102,  FLAG_CLEAR_LAST,
};

struct trial_stage trial_choice;

void trial_reset(void)
{
    trial_choice.stage = 0;
    trial_choice.act = 0;
}

Uint8 trial_stage_count(int story)
{
    const struct trial_stages *list = &trial_stage_lists[story];
    Uint8 i;
    Uint8 count = 0;

    for (i = 0; i < list->count; i++) {
        if (stage_clears[story_characters[story]][list->stages[i].stage] != 0 ||
            menu_state.all_open[0]) {
            count++;
        }
    }
    return count;
}

Uint8 trial_mini_game_count(int story)
{
    const struct trial_stages *list = &trial_mini_game_lists[story];
    Uint8 i;
    Uint8 count = 0;

    for (i = 0; i < list->count; i++) {
        if (stage_clears[story_characters[story]][list->stages[i].stage] != 0 ||
            menu_state.all_open[0]) {
            count++;
        }
    }
    return count;
}

Uint8 trial_stage_total(void)
{
    int story;
    Uint8 total = 0;

    for (story = 0; story < STORY_LAST; story++) {
        total += trial_stage_count(story);
    }
    return total;
}

Uint8 trial_mini_game_total(void)
{
    int story;
    Uint8 total = 0;

    for (story = 0; story < STORY_LAST; story++) {
        total += trial_mini_game_count(story);
    }
    return total;
}

void trial_stage_list(struct trial_stages *list)
{
    const struct trial_stages *all = &trial_stage_lists[menu_state.story];
    Uint8 i;
    Uint8 count = 0;

    for (i = 0; i < all->count; i++) {
        if (stage_clears[story_characters[menu_state.story]]
                        [all->stages[i].stage] != 0 ||
            menu_state.all_open[0]) {
            ((struct trial_stage *)list->stages)[count] = all->stages[i];
            count++;
        }
    }
    list->count = count;
}

void trial_mini_game_list(struct trial_stages *list)
{
    const struct trial_stages *all = &trial_mini_game_lists[menu_state.story];
    Uint8 i;
    Uint8 count = 0;

    for (i = 0; i < all->count; i++) {
        if (stage_clears[story_characters[menu_state.story]]
                        [all->stages[i].stage] != 0 ||
            menu_state.all_open[0]) {
            ((struct trial_stage *)list->stages)[count] = all->stages[i];
            count++;
        }
    }
    list->count = count;
}

int character_playable(int story)
{
    if (menu_state.all_open[0]) {
        return 1;
    }
    switch (story) {
    case STORY_SONIC:
        story = event_flag_check(FLAG_PLAYABLE_SONIC);
        break;
    case STORY_TAILS:
        story = event_flag_check(FLAG_PLAYABLE_TAILS);
        break;
    case STORY_KNUCKLES:
        story = event_flag_check(FLAG_PLAYABLE_KNUCKLES);
        break;
    case STORY_AMY:
        story = event_flag_check(FLAG_PLAYABLE_AMY);
        break;
    case STORY_BIG:
        story = event_flag_check(FLAG_PLAYABLE_BIG);
        break;
    case STORY_E102:
        story = event_flag_check(FLAG_PLAYABLE_E102);
        break;
    case STORY_LAST:
        story = event_flag_check(FLAG_ALL_CLEAR);
        break;
    }
    return story;
}

Bool internet_open(void)
{
    if (menu_state.all_open[0]) {
        return TRUE;
    }
    return save_isp_found();
}

Bool story_cleared(int story)
{
    if (menu_state.all_open[0]) {
        return TRUE;
    }
    if (event_flag_check(story_end_flags[story])) {
        return TRUE;
    }
    return FALSE;
}

/* @hack type: set_character_number and stage_set_stage_act declared again,
 * under names of their own and without a prototype. a call then passes each
 * argument zero-extended to an int (EXTU.W at 0x8c90127c, and EXTU.B at
 * 0x8c9012f0 and 0x8c9012f4). called through stage.h's prototypes,
 * advertise_apply runs 4 bytes shorter, the functions after it move, and 72 of
 * its 100 words differ */
/* NOLINTBEGIN(clang-diagnostic-strict-prototypes) */
extern void menus_set_character_number();
extern void menus_set_stage_act();
/* NOLINTEND(clang-diagnostic-strict-prototypes) */

void advertise_apply(void)
{
    struct trial_stage choice;
    int stage;

    /* @hack type: record_voice_only's result cast to Sint8, which SHC
     * sign-extends from its low byte (EXTS.B at 0x8c901252). without the cast,
     * the function runs 2 bytes shorter, the functions after it move, and 90
     * of 100 words differ */
    messages_shown = (Sint8)record_voice_only() ? FALSE : TRUE;
    last_story = FALSE;
    if (menu_state.story == STORY_LAST) {
        last_story = TRUE;
    }
    /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
    menus_set_character_number(story_characters[menu_state.story]);
    choice = trial_choice;
    stage = choice.stage;
    /* in the mini game trial, four stories' bosses start at another boss than
     * the one their list names, at act 0 */
    if (menu_state.trial[0]) {
        switch (menu_state.story) {
        case STORY_SONIC:
            if (stage == STAGE_EGGMOBILE3) {
                choice.stage = STAGE_CHAOS0;
                choice.act = 0;
            }
            break;
        case STORY_TAILS:
            if (stage == STAGE_EGGMOBILE2) {
                choice.stage = STAGE_EGGMOBILE1;
                choice.act = 0;
            }
            break;
        case STORY_KNUCKLES:
            if (stage == STAGE_CHAOS6) {
                choice.stage = STAGE_CHAOS2;
                choice.act = 0;
            }
            break;
        case STORY_E102:
            if (stage == STAGE_E101_R) {
                choice.stage = STAGE_E101;
                choice.act = 0;
            }
            break;
        }
    }
    /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
    menus_set_stage_act(choice.stage, choice.act);
    texture_release_characters();
}

void advertise_save_reset(void)
{
    progress_init();
    save_reset_all();
}

void advertise_save_reset_write(void)
{
    save_reset_all_write();
}

int advertise_save_write(void)
{
    return save_write();
}

void advertise_save_read(void)
{
    save_read();
}

Bool advertise_save_file_exists(void)
{
    return save_file_exists();
}

void advertise_save_load(void)
{
    save_load();
}

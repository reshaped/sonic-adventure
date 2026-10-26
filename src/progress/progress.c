/* progress: the event flags, and each character's place in its story */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/people.h"
#include "adv00/station_square.h"
#include "adv0100/people.h"
#include "adv0130/people.h"
#include "adv02/people.h"
#include "adv03/people.h"
#include "event/play.h"
#include "main.h"
#include "object/common.h"
#include "player/player.h"
#include "player/rival.h"
#include "progress/playable.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "save/card.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/bitmap.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/timer.h"

#define TEXT_1_LENGTH 12
#define TEXT_2_LENGTH 18
#define TEXT_3_LENGTH 16
#define TEXT_4_LENGTH 18
#define TEXT_5_LENGTH 18
#define TEXT_6_LENGTH 16
#define TEXT_7_LENGTH 12
#define TEXT_8_LENGTH 14
#define TEXT_9_LENGTH 14
#define TEXT_10_LENGTH 6
#define TEXT_11_LENGTH 16
#define TEXT_12_LENGTH 6
#define TEXT_13_LENGTH 16
#define TEXT_14_LENGTH 10
#define TEXT_15_LENGTH 4
#define TEXT_16_LENGTH 8
#define TEXT_17_LENGTH 8
#define TEXT_18_LENGTH 8
#define TEXT_19_LENGTH 8
#define TEXT_20_LENGTH 16
#define TEXT_21_LENGTH 16
#define TEXT_22_LENGTH 16
#define TEXT_23_LENGTH 16
#define TEXT_24_LENGTH 8
#define TEXT_25_LENGTH 10
#define TEXT_26_LENGTH 12
#define TEXT_27_LENGTH 16
#define TEXT_28_LENGTH 16
#define TEXT_29_LENGTH 8
#include "progress/progress/text.inc"

/* the last story runs as Tikal's: story_character and the story_records
 * slot are hers, since she has no story of her own */
#define STORY_LAST CHARACTER_TIKAL

/* the first ACT_FLAGS of event_flags belong to the act: story_start_act
 * clears them as each act starts */
#define ACT_FLAGS 64

/* story_event_delay as a fade out begins. each story_step that doesn't
 * return early takes 1 from the delay. the call that brings it to 1, the
 * 17th such call, plays the held-back event */
#define EVENT_DELAY 18

/* the alpha at which a fade's black covers the screen */
#define ALPHA_OPAQUE 255.0F
/* the alpha a fade moves each step */
#define FADE_STEP 16.0F
/* the fade out's first alpha: 15 steps of FADE_STEP bring it to ALPHA_OPAQUE
 * exactly */
#define FADE_OUT_FIRST 15.0F

/* a battle the story holds: its kind, and the flag a win sets */
struct battle_kind {
    Uint16 kind;
    Uint16 flag;
};

/* a stage, and the flag a character's story sets for clearing it. each
 * character's list ends at a stage of -1 */
struct clear_flag {
    Sint16 stage;
    Sint16 flag;
};

enum battle_mode {
    BATTLE_INIT,
    BATTLE_WAIT, /* until player 0's task exists */
    BATTLE_BEGUN,
    BATTLE_END
};

enum story_fade_mode { STORY_FADE_INIT, STORY_FADE_RUN, STORY_FADE_END };

struct story_fade_vars {
    Uint8 unk_00[8];
    /* the black's alpha: 0 shows the screen, and ALPHA_OPAQUE covers it */
    Float alpha;
};

#define VARS(work) ((struct story_fade_vars *)&(work)->vars)

static Bool story_in_field(void);
static void story_save_wait(void);
static void battle_step(struct task *task);
static void battle_finish(struct task *task);
static void story_fade_draw(struct task *task);
static void story_fade_destroy(struct task *task);
static void story_fade_in_step(struct task *task);
static void story_fade_out_step(struct task *task);
static void story_complete(void);
static void last_story_restart(void);

/* TRUE while the story runs. a demo, and a stage the title's debug menu
 * starts, run without it */
static Bool story_enabled = TRUE;
Bool last_story = FALSE;
int battle_rival = -1;

/* the events story_play_event holds back behind a fade out to black, ending
 * at -1 */
static int story_fade_out_events[] = {
    7,   40,  19,  51,  56,  59,  66,  69,  99,  104, 110, 133, 138,
    143, 146, 151, 177, 178, 186, 220, 224, 248, 250, 13,  -1,
};

/* the events that fade in from black as they start, ending at -1 */
static int story_fade_in_events[] = {
    7,   40,  11,  19,  27,  51,  56,  57,  59,  66,  69,  76,  99,
    104, 110, 133, 138, 139, 143, 146, 151, 177, 178, 186, 194, 220,
    224, 248, 250, 12,  58,  77,  97,  109, 128, 130, 140, 148, 157,
    159, 216, 226, 242, 17,  41,  62,  84,  106, 180, 357, 358, 359,
    360, 361, 362, 363, 364, 365, 366, 367, 368, 369, 374, 375, 376,
    377, 378, 379, 380, 1,   8,   13,  30,  52,  219, 221, -1,
};

/* each stage's name in Japanese, in Shift JIS, by stage number. the fields
 * and the cart have none. no code reads the table */
static char *japanese_stage_names[] = {
    TEXT_1,  TEXT_2,  TEXT_3,  TEXT_4,  TEXT_5,  TEXT_6,  TEXT_7,  TEXT_8,
    TEXT_9,  TEXT_10, TEXT_11, TEXT_12, TEXT_13, TEXT_14, TEXT_15, TEXT_16,
    TEXT_17, TEXT_18, TEXT_19, TEXT_20, TEXT_21, TEXT_22, TEXT_23, TEXT_24,
    TEXT_25, TEXT_26, NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    TEXT_27, TEXT_28, TEXT_29,
};

/* the battles the story holds, by the number battle_start takes: each one's
 * kind and flag here, and its rival in battle_rivals */
static struct battle_kind battle_kinds[] = {
    {BATTLE_KNUCKLES_SONIC, FLAG_SONIC_MBOSS_KNUCKLES},
    {BATTLE_E102_SONIC, FLAG_SONIC_MBOSS_E102},
    {BATTLE_KNUCKLES_TAILS, FLAG_TAILS_MBOSS_KNUCKLES},
    {BATTLE_E102_TAILS, FLAG_TAILS_MBOSS_E102},
    {BATTLE_SONIC_KNUCKLES, FLAG_KNUCKLES_MBOSS_SONIC},
    {BATTLE_SONIC_E102, FLAG_E102_MBOSS_SONIC},
};
static Uint16 battle_rivals[] = {
    CHARACTER_KNUCKLES, CHARACTER_E102,  CHARACTER_KNUCKLES,
    CHARACTER_E102,     CHARACTER_SONIC, CHARACTER_SONIC,
};

static struct clear_flag sonic_clear_flags[] = {
    {STAGE_BEACH, FLAG_SONIC_CLEAR_BEACH},
    {STAGE_WINDY, FLAG_SONIC_CLEAR_WINDY},
    {STAGE_CASINO, FLAG_SONIC_CLEAR_CASINO},
    {STAGE_TWINKLE, FLAG_SONIC_CLEAR_TWINKLE_PARK},
    {STAGE_HIGHWAY, FLAG_SONIC_CLEAR_HIGHWAY},
    {STAGE_MOUNTAIN, FLAG_SONIC_CLEAR_MOUNTAIN},
    {STAGE_ICECAP, FLAG_SONIC_CLEAR_ICECAP},
    {STAGE_SKY_DECK, FLAG_SONIC_CLEAR_SKY_DECK},
    {STAGE_RUIN, FLAG_SONIC_CLEAR_RUIN},
    {STAGE_FINAL, FLAG_SONIC_CLEAR_FINAL_EGG},
    {STAGE_CHAOS0, FLAG_SONIC_CLEAR_CHAOS0},
    {STAGE_CHAOS4, FLAG_SONIC_CLEAR_CHAOS4},
    {STAGE_CHAOS6, FLAG_SONIC_CLEAR_CHAOS6},
    {STAGE_EGGMOBILE1, FLAG_SONIC_CLEAR_EGGMOBILE1},
    {STAGE_EGGMOBILE3, FLAG_SONIC_CLEAR_EGGMOBILE3},
    {STAGE_SHOOTING1, FLAG_SONIC_CLEAR_SHOOTING1},
    {STAGE_SHOOTING2, FLAG_SONIC_CLEAR_SHOOTING2},
    {-1, -1},
};

static struct clear_flag tails_clear_flags[] = {
    {STAGE_WINDY, FLAG_TAILS_CLEAR_WINDY},
    {STAGE_CASINO, FLAG_TAILS_CLEAR_CASINO},
    {STAGE_HIGHWAY, FLAG_TAILS_CLEAR_HIGHWAY},
    {STAGE_ICECAP, FLAG_TAILS_CLEAR_ICECAP},
    {STAGE_SKY_DECK, FLAG_TAILS_CLEAR_SKY_DECK},
    {STAGE_SUNABOARD, FLAG_TAILS_CLEAR_SAND_BOARD},
    {STAGE_CHAOS4, FLAG_TAILS_CLEAR_CHAOS4},
    {STAGE_EGGMOBILE1, FLAG_TAILS_CLEAR_EGGMOBILE1},
    /* @bug logic: clearing the Egg Walker sets no flag. Tails fights it in
     * STAGE_EGGMOBILE2, but this row pairs its flag with STAGE_EGGMOBILE3 (22
     * and 232 at 0x8c1d6634) */
    {STAGE_EGGMOBILE3, FLAG_TAILS_CLEAR_EGGMOBILE2},
    {STAGE_SHOOTING1, FLAG_TAILS_CLEAR_SHOOTING1},
    {STAGE_SHOOTING2, FLAG_TAILS_CLEAR_SHOOTING2},
    {STAGE_BEACH, FLAG_TAILS_CLEAR_BEACH},
    {STAGE_MOUNTAIN, FLAG_TAILS_CLEAR_MOUNTAIN},
    {-1, -1},
};

/* @bug memory: Knuckles' list has no stage of -1 to end it. for a
 * stage it lacks, story_mark_cleared's walk (CMP/EQ #-1 at 0x8c095aac) reads
 * on into amy_clear_flags, which follows it at 0x8c1d666c, so a stage Amy's
 * list holds sets her flag for it */
static struct clear_flag knuckles_clear_flags[] = {
    {STAGE_CASINO, FLAG_KNUCKLES_CLEAR_CASINO},
    {STAGE_HIGHWAY, FLAG_KNUCKLES_CLEAR_HIGHWAY},
    {STAGE_MOUNTAIN, FLAG_KNUCKLES_CLEAR_MOUNTAIN},
    {STAGE_SKY_DECK, FLAG_KNUCKLES_CLEAR_SKY_DECK},
    {STAGE_RUIN, FLAG_KNUCKLES_CLEAR_RUIN},
    {STAGE_CHAOS2, FLAG_KNUCKLES_CLEAR_CHAOS2},
    {STAGE_CHAOS4, FLAG_KNUCKLES_CLEAR_CHAOS4},
    {STAGE_CHAOS6, FLAG_KNUCKLES_CLEAR_CHAOS6},
};

static struct clear_flag amy_clear_flags[] = {
    {STAGE_TWINKLE, FLAG_AMY_CLEAR_TWINKLE_PARK},
    {STAGE_SHELTER, FLAG_AMY_CLEAR_HOT_SHELTER},
    {STAGE_FINAL, FLAG_AMY_CLEAR_FINAL_EGG},
    {STAGE_EGGMANROBO, FLAG_AMY_CLEAR_EGGMANROBO},
    {-1, -1},
};

static struct clear_flag e102_clear_flags[] = {
    {STAGE_BEACH, FLAG_E102_CLEAR_BEACH},
    {STAGE_WINDY, FLAG_E102_CLEAR_WINDY},
    {STAGE_MOUNTAIN, FLAG_E102_CLEAR_MOUNTAIN},
    {STAGE_SHELTER, FLAG_E102_CLEAR_HOT_SHELTER},
    {STAGE_FINAL, FLAG_E102_CLEAR_FINAL_EGG},
    {STAGE_E101, FLAG_E102_CLEAR_E101},
    {STAGE_E101_R, FLAG_E102_CLEAR_E101_R},
    {-1, -1},
};

static struct clear_flag big_clear_flags[] = {
    {STAGE_BEACH, FLAG_BIG_CLEAR_BEACH},
    {STAGE_SHELTER, FLAG_BIG_CLEAR_HOT_SHELTER},
    {STAGE_TWINKLE, FLAG_BIG_CLEAR_TWINKLE_PARK},
    {STAGE_ICECAP, FLAG_BIG_CLEAR_ICECAP},
    {STAGE_CHAOS6, FLAG_BIG_CLEAR_CHAOS6},
    {-1, -1},
};

static struct clear_flag *story_clear_flags[CHARACTER_COUNT] = {
    sonic_clear_flags,
    NULL,
    tails_clear_flags,
    knuckles_clear_flags,
    NULL,
    amy_clear_flags,
    e102_clear_flags,
    big_clear_flags,
};

/* each character's flag for the Egg Carrier's sinking, which
 * egg_carrier_has_sunk tests. Eggman and Tikal have 0 in its place */
static Sint16 egg_carrier_sink_flags[CHARACTER_COUNT] = {
    FLAG_SONIC_EC_SINK,
    0,
    FLAG_TAILS_EC_SINK,
    FLAG_KNUCKLES_EC_SINK,
    0,
    FLAG_AMY_EC_SINK,
    FLAG_E102_EC_SINK,
    FLAG_BIG_EC_SINK,
};

/* the last story's events, which last_story_restart marks unplayed so they
 * play again. the list ends at 0 */
static int last_story_events[] = {
    240, 242, 243, 244, 245, 246, 247, 248,
    249, 250, 251, 253, 254, 255, 352, 0,
};

Sint8 stage_clears[CHARACTER_COUNT][STAGE_COUNT];
Uint8 event_flags[FLAG_COUNT];
struct story_record story_records[CHARACTER_COUNT];
Uint8 dat_8c7a2718[CHARACTER_COUNT];
Sint8 events_played[EVENT_COUNT];
Uint8 dat_8c7a2920[512];
Uint8 dat_8c7a2b20[512];
Uint8 dat_8c7a2d20[16];
Uint8 dat_8c7a2d30[512];
Uint8 dat_8c7a2f30[344];
struct story_section *story_table;
struct story_script *story_current_script;
Sint16 story_character;
int story_steps;
Bool story_ended;
int story_event_delay;
/* the event story_play_event held back, which story_step plays as
 * story_event_delay runs out */
static int story_held_event;
/* no code names it, but it holds its place in bss */
static Uint8 dat_8c7a30a4[140];
/* the battle battle_start holds, by number, and the record battle_step fills
 * for it */
static int battle_number;
static struct battle battle_record;

void progress_init(void)
{
    Uint8 *byte;
    int i;

    /* @hack spelling: each clear walks a byte pointer and counts down, which
     * SHC writes as DT (0x8c095008 to 0x8c095054); subscripted, 60 of 60 words
     * differ */
    for (byte = (Uint8 *)story_records, i = sizeof(story_records); i != 0;
         i--) {
        *byte++ = 0;
    }
    for (byte = event_flags, i = sizeof(event_flags); i != 0; i--) {
        *byte++ = 0;
    }
    for (byte = (Uint8 *)stage_clears, i = sizeof(stage_clears); i != 0; i--) {
        *byte++ = 0;
    }
    /* this clear runs 96 bytes, sizeof story_records, from dat_8c7a2d20: its
     * own 16 and the first 80 of dat_8c7a2d30 */
    for (byte = dat_8c7a2d20, i = sizeof(story_records); i != 0; i--) {
        *byte++ = 0;
    }
    for (byte = dat_8c7a2b20, i = sizeof(dat_8c7a2b20); i != 0; i--) {
        *byte++ = 0;
    }
    for (byte = dat_8c7a2f30, i = sizeof(dat_8c7a2f30); i != 0; i--) {
        *byte++ = 0;
    }
    for (byte = dat_8c7a2d30, i = sizeof(dat_8c7a2d30); i != 0; i--) {
        *byte++ = 0;
    }
    player_record = NULL;
    story_current_script = NULL;
    playable_set(CHARACTER_SONIC);
    event_init();
    story_event_delay = 0;
}

void story_clear(void)
{
    player_record = NULL;
    story_current_script = NULL;
}

void story_enable(void)
{
    story_enabled = TRUE;
}

void story_disable(void)
{
    story_enabled = FALSE;
}

Bool story_is_enabled(void)
{
    return story_enabled;
}

void story_set_character(int character)
{
    if (story_is_enabled()) {
        if (last_story) {
            event_flags[FLAG_LAST_STORY] = 1;
            story_character = STORY_LAST;
        } else {
            event_flags[FLAG_LAST_STORY] = 0;
            story_character = character;
        }
        player_record = &story_records[story_character];
        story_table = story_get_table(story_character);
    }
}

void story_set_next_stage(int section)
{
    struct story_record *record;
    struct story_section *first;

    if (!story_is_enabled()) {
        return;
    }
    if (story_table == NULL) {
        return;
    }
    record = player_record;
    if (record->seq == 0 && record->section == 0) {
        first = &story_table[record->section];
        stage_set_entrance(first->entrance);
        stage_set_stage_act(first->stage, first->act);
        /* @bug logic: the store leaves first->stage unshifted (MOV.W
         * R0,@(8,R3) at 0x8c095194). until story_save stores the stage and
         * act, the record reads back as stage 0 and act first->stage */
        player_record->stage_act = first->stage;
        player_record->entrance = first->entrance;
        player_record->wait_at = stage_get_stage_act();
    } else if (record->section < 0) {
        if (story_character == STORY_LAST) {
            last_story_restart();
        } else {
            stage_set_entrance(0);
            stage_set_stage_act(STAGE_STATION_SQUARE, STATION_SQUARE_STATION);
        }
    } else {
        stage_set_stage_act(record->stage_act >> 8, record->stage_act);
        stage_set_entrance(player_record->entrance);
    }
    battle_rival = -1;
}

void story_start_act(void)
{
    struct story_record *record;
    struct story_section *section;
    Uint8 *flag;
    int i;

    for (flag = event_flags, i = 0; i < ACT_FLAGS; i++) {
        *flag++ = 0;
    }
    story_steps = 0;
    story_current_script = NULL;
    if (!story_is_enabled()) {
        return;
    }
    record = player_record;
    if (record == NULL || record->section < 0) {
        return;
    }
    if (!story_in_field()) {
        return;
    }
    section = &story_table[player_record->section];
    story_current_script = section->script;
    if (player_record->seq < 0) {
        player_record->seq = 0;
        player_record->wait_at = STAGE_ACT(section->stage, section->act);
    }
    if (battle_rival != -1) {
        task_create(TASK_WORK, 1, battle_step);
    }
    if (story_current_script->init != NULL) {
        story_current_script->init();
    }
}

/* TRUE if the current stage is the field of the record's section. the Egg
 * Carrier's outside and its inside count as one field */
static Bool story_in_field(void)
{
    int stage = story_table[player_record->section].stage;

    switch (stage_get_stage_act() >> 8) {
    case STAGE_STATION_SQUARE:
        return stage == STAGE_STATION_SQUARE;
    case STAGE_EGG_CARRIER:
    case STAGE_EGG_CARRIER_INSIDE:
        return stage == STAGE_EGG_CARRIER || stage == STAGE_EGG_CARRIER_INSIDE;
    case STAGE_MYSTIC_RUINS:
        return stage == STAGE_MYSTIC_RUINS;
    case STAGE_PAST:
        return stage == STAGE_PAST;
    }
    return FALSE;
}

void story_step(void)
{
    int *list;
    int found;
    int before;

    story_ended = FALSE;
    if (story_steps++ < 2) {
        return;
    }
    if (!story_is_enabled()) {
        return;
    }
    if (player_record == NULL) {
        return;
    }
    if (story_current_script == NULL) {
        return;
    }
    if (playable_notice_active()) {
        return;
    }
    if (bitmap_pages_active()) {
        return;
    }
    if (event_in_control()) {
        return;
    }
    /* @hack redundant: the store of STORY_NO_WAIT in an else, which puts it
     * through R3 (0x8c095442, 0x8c095444); without it, the store goes through
     * R2 and 2 of 181 words differ */
    if (player_record->wait_at != STORY_NO_WAIT &&
        stage_get_stage_act() != player_record->wait_at) {
        return;
    } else {
        player_record->wait_at = STORY_NO_WAIT;
    }
    if (player_record->section == -1) {
        story_ended = TRUE;
    }
    if (story_event_delay != 0) {
        story_event_delay--;
        if (story_event_delay == 1) {
            event_load(story_held_event);
            event_start();
            found = 0;
            /* @hack redundant: || found, always 0 where the list ends, keeps
             * the image's test of found there (0x8c095486, 0x8c09549e);
             * without it, 145 of 181 words differ */
            for (list = story_fade_in_events; *list >= 0 || found; list++) {
                if (story_held_event == *list) {
                    found = -1;
                    break;
                }
            }
            if (found) {
                task_create(TASK_WORK, 1, story_fade_in_step);
            } else {
                draw_off = FALSE;
            }
            story_event_delay = 0;
            before = player_record->seq;
            story_current_script->step(player_record->seq);
            if (player_record->seq != -1 && before == player_record->seq) {
                story_next_seq();
            }
        }
    } else {
        story_current_script->step(player_record->seq);
    }
}

void story_save(void)
{
    struct story_record *record = player_record;

    if (record != NULL) {
        record->stage_act = stage_get_stage_act();
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        player_record->entrance = stage_get_entrance();
        save_write();
    }
}

static void story_save_wait(void)
{
    struct story_record *record = player_record;

    if (record != NULL) {
        record->stage_act = stage_get_stage_act();
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        player_record->entrance = stage_get_entrance();
        save_write_wait();
    }
}

void fun_8c0955fc(void)
{
}

void story_set_wait(Uint16 stage_act)
{
    struct story_record *record = player_record;

    if (record != NULL) {
        record->wait_at = stage_act;
    }
}

void story_next_seq(void)
{
    struct story_record *record = player_record;

    if (record->seq < 0) {
        return;
    }
    if (story_event_delay == 0) {
        record->seq++;
    }
}

void story_next_section(void)
{
    if (story_event_delay != 0) {
        return;
    }
    player_record->seq = -1;
    player_record->section++;
    if (player_record->section >= story_get_length(story_character)) {
        story_complete();
        player_record->section = -1;
        story_save_wait();
    }
}

void story_play_event(int event)
{
    int *list;
    int found;

    if (event_was_played(event)) {
        return;
    }
    found = 0;
    if (story_event_delay != 0) {
        return;
    }
    /* @hack redundant: || found, always 0 where the list ends, keeps the
     * image's test of found there (0x8c0956d4, 0x8c0956ea); without it, 88 of
     * 101 words differ */
    for (list = story_fade_out_events; *list >= 0 || found; list++) {
        if (event == *list) {
            found = -1;
            break;
        }
    }
    if (found) {
        task_create(TASK_WORK, 1, story_fade_out_step);
        pad_disable();
        story_event_delay = EVENT_DELAY;
        story_held_event = event;
    } else {
        story_event_delay = 0;
    }
    if (story_event_delay != 0) {
        return;
    }
    event_load(event);
    event_start();
    found = 0;
    /* @bug logic: the search tests story_held_event, the last event a
     * fade out held back, not event, which this call plays (MOV.L @R10,R1 at
     * 0x8c09572e). until a held-back event has played, no event played here
     * fades in. after that, every one does */
    /* @hack redundant: || found, always 0 where the list ends, keeps the
     * image's test of found there (0x8c09572a, 0x8c095742); without it, 57 of
     * 101 words differ */
    for (list = story_fade_in_events; *list >= 0 || found; list++) {
        if (story_held_event == *list) {
            found = -1;
            break;
        }
    }
    if (found) {
        task_create(TASK_WORK, 1, story_fade_in_step);
    } else {
        draw_off = FALSE;
    }
}

void story_set_time(Sint8 time)
{
    struct story_record *record = player_record;

    if (record != NULL) {
        record->time = time;
    }
}

int story_get_time(void)
{
    struct story_record *record = player_record;

    if (record == NULL) {
        return TIME_DAY;
    }
    return record->time;
}

Bool playable_check(int character)
{
    switch (character) {
    case CHARACTER_SONIC:
        return event_flags[FLAG_PLAYABLE_SONIC];
    case CHARACTER_TAILS:
        return event_flags[FLAG_PLAYABLE_TAILS];
    case CHARACTER_KNUCKLES:
        return event_flags[FLAG_PLAYABLE_KNUCKLES];
    case CHARACTER_AMY:
        return event_flags[FLAG_PLAYABLE_AMY];
    case CHARACTER_E102:
        return event_flags[FLAG_PLAYABLE_E102];
    case CHARACTER_BIG:
        return event_flags[FLAG_PLAYABLE_BIG];
    }
    return FALSE;
}

void playable_set(int character)
{
    switch (character) {
    case CHARACTER_SONIC:
        event_flags[FLAG_PLAYABLE_SONIC] = 1;
        break;
    case CHARACTER_TAILS:
        event_flags[FLAG_PLAYABLE_TAILS] = 1;
        break;
    case CHARACTER_KNUCKLES:
        event_flags[FLAG_PLAYABLE_KNUCKLES] = 1;
        break;
    case CHARACTER_AMY:
        event_flags[FLAG_PLAYABLE_AMY] = 1;
        break;
    case CHARACTER_E102:
        event_flags[FLAG_PLAYABLE_E102] = 1;
        break;
    case CHARACTER_BIG:
        event_flags[FLAG_PLAYABLE_BIG] = 1;
        break;
    }
}

void event_flag_set(int flag)
{
    event_flags[flag] = 1;
}

void event_flag_clear(int flag)
{
    event_flags[flag] = 0;
}

Bool event_flag_check(int flag)
{
    return event_flags[flag] != 0;
}

Bool story_wait_at(int stage, int act)
{
    int stage_act = STAGE_ACT(stage, act);

    if (stage_get_stage_act() != stage_act) {
        player_record->wait_at = stage_act;
        return FALSE;
    }
    return TRUE;
}

void fun_8c0958e8(void)
{
}

void story_go_to(int stage, int act)
{
    stage_fade_to(stage, act);
}

Bool story_going_to(Uint8 stage, Uint8 act)
{
    player_record->wait_at = STORY_NO_WAIT;
    return stage_get_next_stage_act() == STAGE_ACT(stage, act);
}

/* the task that holds a battle in the field. it takes the field's people
 * away, and once player 0's task exists, creates the rival and begins the
 * battle */
static void battle_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case BATTLE_INIT:
        stream_destroy();
        field_people_remove();
        sound_load_alt_voice();
        work->mode = BATTLE_WAIT;
        /* fall through */
    case BATTLE_WAIT:
        if (player_get_task(0) != NULL) {
            battle_record.kind = battle_kinds[battle_number].kind;
            battle_record.player = player_get_task(0);
            battle_record.rival = NULL;
            battle_record.unk_0c = 0;
            battle_record.finish = battle_finish;
            battle_create_rival(&battle_record);
            battle_begin(&battle_record);
            work->mode = BATTLE_BEGUN;
        }
        break;
    case BATTLE_BEGUN:
        work->mode = BATTLE_END;
        break;
    case BATTLE_END:
        task_kill(task);
        break;
    }
}

/* battle_record's finish callback. a win sets the battle's flag and clears
 * battle_rival, so later acts hold no battle. win or lose, the rival goes */
static void battle_finish(struct task *task)
{
    int flag;

    stream_destroy();
    if (battle_outcome() == BATTLE_WON) {
        flag = battle_kinds[battle_number].flag;
        event_flags[flag] = 1;
        battle_rival = -1;
    }
    battle_remove_rival(&battle_record);
    sound_load_alt_field();
}

void battle_start(int number)
{
    battle_rival = battle_rivals[number];
    battle_number = number;
    task_create(TASK_WORK, 1, battle_step);
}

Bool stage_cleared(int stage)
{
    return stage_clears[get_character()][stage] != 0;
}

void story_mark_cleared(int stage)
{
    struct clear_flag *entry;

    for (entry = story_clear_flags[get_character()]; entry->stage != -1;
         entry++) {
        if (entry->stage == stage) {
            event_flags[entry->flag] = 1;
            return;
        }
    }
}

Bool egg_carrier_has_sunk(void)
{
    return event_flag_check(egg_carrier_sink_flags[get_character()]);
}

void field_people_place(void)
{
    switch ((Uint8)(stage_get_stage_act() >> 8)) {
    case STAGE_STATION_SQUARE:
        station_square_people_place();
        break;
    case STAGE_EGG_CARRIER:
        egg_carrier_people_place();
        break;
    case STAGE_EGG_CARRIER_INSIDE:
        egg_carrier_inside_people_place();
        break;
    case STAGE_MYSTIC_RUINS:
        mystic_ruins_people_place();
        break;
    case STAGE_PAST:
        past_people_place();
        break;
    }
}

void field_people_remove(void)
{
    switch ((Uint8)(stage_get_stage_act() >> 8)) {
    case STAGE_STATION_SQUARE:
        station_square_people_remove();
        break;
    case STAGE_EGG_CARRIER:
        egg_carrier_people_remove();
        break;
    case STAGE_EGG_CARRIER_INSIDE:
        egg_carrier_inside_people_remove();
        break;
    case STAGE_MYSTIC_RUINS:
        mystic_ruins_people_remove();
        break;
    case STAGE_PAST:
        past_people_remove();
        break;
    }
}

/* draw black over the whole screen at the fade's alpha */
static void story_fade_draw(struct task *task)
{
    NJS_POINT2COL quad;
    NJS_POINT2 points[4];
    NJS_COLOR colors[4];
    NJS_COLOR color;
    struct task_work *work = task->work;

    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    quad.p = points;
    quad.col = colors;
    quad.tex = NULL;
    quad.num = 4;
    color.argb.b = color.argb.g = color.argb.r = 0;
    color.argb.a = VARS(work)->alpha;
    quad.p[0].x = 0.0F;
    quad.p[0].y = 0.0F;
    quad.col[0].color = color.color;
    quad.p[1].x = SCREEN_WIDTH;
    quad.p[1].y = 0.0F;
    quad.col[1].color = color.color;
    quad.p[2].x = SCREEN_WIDTH;
    quad.p[2].y = SCREEN_HEIGHT;
    quad.col[2].color = color.color;
    quad.p[3].x = 0.0F;
    quad.p[3].y = SCREEN_HEIGHT;
    quad.col[3].color = color.color;
    njDrawPolygon2D(&quad, 4, -0.1F,
                    NJD_FILL | NJD_TRANSPARENT | NJD_DRAW_CONNECTED);
}

static void story_fade_destroy(struct task *task)
{
}

/* the task that fades in from black as an event starts: alpha falls by
 * FADE_STEP each step from ALPHA_OPAQUE to 0 */
static void story_fade_in_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case STORY_FADE_INIT:
        task->draw = story_fade_draw;
        task->destructor = story_fade_destroy;
        work->mode = STORY_FADE_RUN;
        VARS(work)->alpha = ALPHA_OPAQUE;
        break;
    case STORY_FADE_RUN:
        draw_off = FALSE;
        story_fade_draw(task);
        VARS(work)->alpha -= FADE_STEP;
        if (VARS(work)->alpha < 0.0F) {
            VARS(work)->alpha = 0.0F;
            work->mode = STORY_FADE_END;
        }
        break;
    case STORY_FADE_END:
        task_kill(task);
        break;
    }
}

/* the task that fades out to black before a held-back event plays: alpha
 * rises by FADE_STEP each step to ALPHA_OPAQUE, where it sets draw_off */
static void story_fade_out_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case STORY_FADE_INIT:
        task->draw = story_fade_draw;
        task->destructor = story_fade_destroy;
        work->mode = STORY_FADE_RUN;
        VARS(work)->alpha = FADE_OUT_FIRST;
        /* fall through */
    case STORY_FADE_RUN:
        story_fade_draw(task);
        VARS(work)->alpha += FADE_STEP;
        if (VARS(work)->alpha > ALPHA_OPAQUE) {
            VARS(work)->alpha = ALPHA_OPAQUE;
            work->mode = STORY_FADE_END;
            draw_off = TRUE;
        }
        break;
    case STORY_FADE_END:
        story_fade_draw(task);
        task_kill(task);
        break;
    }
}

/* end the character's story by setting its flags: its FLAG_CLEAR_ flag,
 * and its _TRAIN, _BOAT, _RAFT and _UNK_ flags. Knuckles and Big also set
 * their _EC_SINK flags. FLAG_ALL_CLEAR follows once all six stories have
 * ended. the last story sets none of them */
static void story_complete(void)
{
    if (story_character == STORY_LAST) {
        return;
    }
    switch (story_character) {
    case CHARACTER_SONIC:
        event_flags[FLAG_CLEAR_SONIC] = 1;
        event_flags[FLAG_SONIC_TRAIN] = 1;
        event_flags[FLAG_SONIC_BOAT] = 1;
        event_flags[FLAG_SONIC_RAFT] = 1;
        event_flags[FLAG_SONIC_UNK_82] = 1;
        break;
    case CHARACTER_TAILS:
        event_flags[FLAG_CLEAR_TAILS] = 1;
        event_flags[FLAG_TAILS_TRAIN] = 1;
        event_flags[FLAG_TAILS_BOAT] = 1;
        event_flags[FLAG_TAILS_RAFT] = 1;
        event_flags[FLAG_TAILS_UNK_83] = 1;
        break;
    case CHARACTER_KNUCKLES:
        event_flags[FLAG_KNUCKLES_EC_SINK] = 1;
        event_flags[FLAG_CLEAR_KNUCKLES] = 1;
        event_flags[FLAG_KNUCKLES_TRAIN] = 1;
        event_flags[FLAG_KNUCKLES_BOAT] = 1;
        event_flags[FLAG_KNUCKLES_RAFT] = 1;
        event_flags[FLAG_KNUCKLES_UNK_84] = 1;
        break;
    case CHARACTER_AMY:
        event_flags[FLAG_CLEAR_AMY] = 1;
        event_flags[FLAG_AMY_TRAIN] = 1;
        event_flags[FLAG_AMY_BOAT] = 1;
        event_flags[FLAG_AMY_RAFT] = 1;
        event_flags[FLAG_AMY_UNK_85] = 1;
        break;
    case CHARACTER_BIG:
        event_flags[FLAG_BIG_EC_SINK] = 1;
        event_flags[FLAG_CLEAR_BIG] = 1;
        event_flags[FLAG_BIG_TRAIN] = 1;
        event_flags[FLAG_BIG_BOAT] = 1;
        event_flags[FLAG_BIG_RAFT] = 1;
        event_flags[FLAG_BIG_UNK_87] = 1;
        break;
    case CHARACTER_E102:
        event_flags[FLAG_CLEAR_E102] = 1;
        event_flags[FLAG_E102_TRAIN] = 1;
        event_flags[FLAG_E102_BOAT] = 1;
        event_flags[FLAG_E102_RAFT] = 1;
        event_flags[FLAG_E102_UNK_86] = 1;
        break;
    }
    if (!event_flag_check(FLAG_ALL_CLEAR)) {
        if (event_flag_check(FLAG_CLEAR_SONIC) &&
            event_flag_check(FLAG_CLEAR_TAILS) &&
            event_flag_check(FLAG_CLEAR_KNUCKLES) &&
            event_flag_check(FLAG_CLEAR_AMY) &&
            event_flag_check(FLAG_CLEAR_BIG) &&
            event_flag_check(FLAG_CLEAR_E102)) {
            event_flags[FLAG_ALL_CLEAR] = 1;
        }
    }
}

/* start the last story over at its first section, and mark
 * last_story_events unplayed so they play again */
static void last_story_restart(void)
{
    struct story_section *section;
    int i;

    player_record->section = 0;
    player_record->seq = 0;
    section = story_table;
    stage_set_entrance(section->entrance);
    stage_set_stage_act(section->stage, section->act);
    player_record->wait_at = stage_get_stage_act();
    for (i = 0; last_story_events[i] != 0; i++) {
        events_played[last_story_events[i]] = 0;
    }
}

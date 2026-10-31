/* saves: a game's progress and records, as the memory card keeps them */
#include <sg_xpt.h>
#include <string.h>

#include "hud/hud.h"
#include "player/player.h"
#include "progress/progress.h"
#include "save/card.h"
#include "save/minigame.h"
#include "save/save.h"
#include "stage/stage.h"

/* the stage records before E-102's, whose best time is the least. E-102's
 * keep the time left, so their best is the most */
#define LEAST_TIMES 23

/* the CRC-16-CCITT polynomial, reflected for a CRC taken low bit first */
#define CRC_POLYNOMIAL 0x8408

/* a character's action stages, as STAGE_ACTs in the order a save keeps
 * their records */
struct save_stage_list {
    Uint16 *stage_acts;
    int count;
};

static Uint16 save_checksum(struct save *save);
static void pack_event_flags(struct save *save);
static void pack_events_played(struct save *save);
static void store_unk_2e0(struct save *save);
static void store_stage_clears(struct save *save);
static void store_story(struct save *save);
static void unpack_event_flags(struct save *save);
static void unpack_events_played(struct save *save);
static void load_unk_2e0(struct save *save);
static void load_stage_clears(struct save *save);
static void load_story(struct save *save);

static Uint16 sonic_stages[] = {
    STAGE_ACT(STAGE_BEACH, 0),    STAGE_ACT(STAGE_WINDY, 0),
    STAGE_ACT(STAGE_TWINKLE, 0),  STAGE_ACT(STAGE_HIGHWAY, 0),
    STAGE_ACT(STAGE_MOUNTAIN, 0), STAGE_ACT(STAGE_SKY_DECK, 0),
    STAGE_ACT(STAGE_RUIN, 0),     STAGE_ACT(STAGE_ICECAP, 0),
    STAGE_ACT(STAGE_CASINO, 0),   STAGE_ACT(STAGE_FINAL, 0),
};
static Uint16 tails_stages[] = {
    STAGE_ACT(STAGE_WINDY, 2),    STAGE_ACT(STAGE_HIGHWAY, 0),
    STAGE_ACT(STAGE_SKY_DECK, 0), STAGE_ACT(STAGE_ICECAP, 2),
    STAGE_ACT(STAGE_CASINO, 1),
};
static Uint16 knuckles_stages[] = {
    STAGE_ACT(STAGE_HIGHWAY, 2),  STAGE_ACT(STAGE_MOUNTAIN, 2),
    STAGE_ACT(STAGE_SKY_DECK, 2), STAGE_ACT(STAGE_RUIN, 1),
    STAGE_ACT(STAGE_CASINO, 0),
};
static Uint16 amy_stages[] = {
    STAGE_ACT(STAGE_TWINKLE, 1),
    STAGE_ACT(STAGE_FINAL, 0),
    STAGE_ACT(STAGE_SHELTER, 0),
};
static Uint16 e102_stages[] = {
    STAGE_ACT(STAGE_BEACH, 0),    STAGE_ACT(STAGE_WINDY, 0),
    STAGE_ACT(STAGE_MOUNTAIN, 1), STAGE_ACT(STAGE_FINAL, 2),
    STAGE_ACT(STAGE_SHELTER, 2),
};
static Uint16 big_stages[] = {
    STAGE_ACT(STAGE_BEACH, 2),
    STAGE_ACT(STAGE_TWINKLE, 1),
    STAGE_ACT(STAGE_ICECAP, 3),
    STAGE_ACT(STAGE_SHELTER, 0),
};

/* each character's index into a save's lives. Eggman and Tikal share
 * Sonic's */
static int lives_index[CHARACTER_COUNT] = {0, 0, 1, 2, 0, 3, 4, 5};

/* each character's list, by its index among a save's records. nothing
 * reads the table */
static struct save_stage_list save_stage_lists[SAVE_CHARACTERS] = {
    {sonic_stages, 10}, {tails_stages, 5}, {knuckles_stages, 5},
    {amy_stages, 3},    {e102_stages, 5},  {big_stages, 4},
};

int stage_last_cleared;
struct save saves[SAVE_COUNT];

static void reset_shooting_scores(struct save *save)
{
    static const struct save_scores act1 = {{8000, 6000, 3000}};
    static const struct save_scores act2 = {{20000, 15000, 10000}};
    struct save_scores *scores;
    int i;

    for (i = 0, scores = save->shooting1_scores; i < 2; i++, scores++) {
        *scores = act1;
    }
    for (i = 0, scores = save->shooting2_scores; i < 2; i++, scores++) {
        *scores = act2;
    }
}

static void reset_mole_scores(struct save *save)
{
    static const struct save_scores defaults = {{2000, 1500, 1000}};
    struct save_scores *scores;

    /* @hack spelling: the scores through a pointer, which adds their offset to
     * save in R4 before the copy (ADD R3,R4 at 0x8c08d180); assigned to
     * save->mole_scores, 9 of 10 words differ */
    scores = &save->mole_scores;
    *scores = defaults;
}

static void reset_trial_times(struct save *save)
{
    static const struct save_times defaults[SAVE_TRIALS] = {
        {{{12, 0, 0}, {13, 30, 0}, {15, 0, 0}}},
        {{{5, 0, 0}, {6, 30, 0}, {8, 0, 0}}},
        {{{6, 30, 0}, {7, 30, 0}, {8, 30, 0}}},
        {{{2, 30, 0}, {3, 0, 0}, {3, 30, 0}}},
        {{{4, 0, 0}, {3, 30, 0}, {3, 0, 0}}},
        {{{0, 15, 0}, {0, 30, 0}, {0, 45, 0}}},
    };
    struct save_times *times;
    int i;

    for (i = 0, times = save->trial_times; i < SAVE_TRIALS; i++, times++) {
        *times = defaults[i];
    }
}

void save_reset_all(void)
{
    struct save save;
    struct save *block;
    int i;

    memset(&save, 0, sizeof(save));
    save.play_time = 0;
    for (i = 0; i < SAVE_STAGES; i++) {
        save.stage_scores[i] = 0;
    }
    for (i = 0; i < LEAST_TIMES; i++) {
        save.stage_times[i].minutes = 99;
        save.stage_times[i].seconds = 59;
        save.stage_times[i].frames = 59;
    }
    for (i = LEAST_TIMES; i < SAVE_STAGE_TIMES; i++) {
        save.stage_times[i].minutes = 0;
        save.stage_times[i].seconds = 0;
        save.stage_times[i].frames = 0;
    }
    board_reset_icecap_scores(&save);
    board_reset_sunaboard_scores(&save);
    cart_reset_times(&save);
    reset_shooting_scores(&save);
    reset_mole_scores(&save);
    reset_trial_times(&save);
    for (i = 0; i < SAVE_CHARACTERS; i++) {
        save.lives[i] = LIVES_START;
    }
    save.event_flags[FLAG_PLAYABLE_SONIC / 8] |= 1 << FLAG_PLAYABLE_SONIC % 8;
    save.story_records[CHARACTER_SONIC].stage_act =
        STAGE_ACT(STAGE_STATION_SQUARE, 0);
    save.stage = NO_STAGE;
    /* @hack spelling: the save through block, which keeps its address in R14
     * across save_checksum (MOV R15,R14 at 0x8c08d384, MOV.L R0,@R14 at
     * 0x8c08d3a0); through save, 40 of 154 words differ */
    block = &save;
    block->checksum = save_checksum(block);
    saves[0] = save;
    saves[1] = save;
    saves[2] = save;
    ring_reset();
}

void save_reset(void)
{
    struct save save;
    struct save *block;
    int i;

    memset(&save, 0, sizeof(save));
    save.play_time = 0;
    for (i = 0; i < SAVE_STAGES; i++) {
        save.stage_scores[i] = 0;
    }
    for (i = 0; i < LEAST_TIMES; i++) {
        save.stage_times[i].minutes = 99;
        save.stage_times[i].seconds = 59;
        save.stage_times[i].frames = 59;
    }
    for (i = LEAST_TIMES; i < SAVE_STAGE_TIMES; i++) {
        save.stage_times[i].minutes = 0;
        save.stage_times[i].seconds = 0;
        save.stage_times[i].frames = 0;
    }
    board_reset_icecap_scores(&save);
    board_reset_sunaboard_scores(&save);
    cart_reset_times(&save);
    reset_shooting_scores(&save);
    reset_mole_scores(&save);
    reset_trial_times(&save);
    for (i = 0; i < SAVE_CHARACTERS; i++) {
        save.lives[i] = LIVES_START;
    }
    save.event_flags[FLAG_PLAYABLE_SONIC / 8] |= 1 << FLAG_PLAYABLE_SONIC % 8;
    save.story_records[CHARACTER_SONIC].stage_act =
        STAGE_ACT(STAGE_STATION_SQUARE, 0);
    save.stage = NO_STAGE;
    /* @hack spelling: the save through block, as in save_reset_all (MOV R15,R14
     * at 0x8c08d504); through save, 34 of 148 words differ */
    block = &save;
    block->checksum = save_checksum(block);
    saves[save_slot] = save;
    ring_reset();
}

void save_store(void)
{
    struct save save;
    struct save *block;

    save = saves[save_slot];
    save.play_time = play_time;
    save.lives[lives_index[get_character()]] = lives;
    save.character = get_character();
    save.stage = stage_last_cleared;
    pack_event_flags(&save);
    pack_events_played(&save);
    store_unk_2e0(&save);
    store_story(&save);
    store_stage_clears(&save);
    /* @hack spelling: the save through block, as in save_reset_all (MOV R15,R14
     * at 0x8c08d604); through save, 72 of 72 words differ */
    block = &save;
    block->checksum = save_checksum(block);
    saves[save_slot] = save;
}

void save_update(void)
{
    struct save save;
    struct save *block;

    save = saves[save_slot];
    save.play_time = play_time;
    save.character = get_character();
    save.stage = stage_last_cleared;
    /* @hack spelling: the save through block, as in save_reset_all (MOV R15,R14
     * at 0x8c08d6ac); through save, 50 of 50 words differ */
    block = &save;
    block->checksum = save_checksum(block);
    saves[save_slot] = save;
}

void save_load(void)
{
    struct save save;

    save = saves[save_slot];
    play_time = save.play_time;
    lives = save.lives[lives_index[save.character]];
    set_character_number(save.character);
    stage_last_cleared = save.stage;
    unpack_event_flags(&save);
    unpack_events_played(&save);
    load_unk_2e0(&save);
    load_story(&save);
    load_stage_clears(&save);
}

/* the CRC-16 of the bytes of *save after its checksum, from 0xffff and
 * returned inverted */
static Uint16 save_checksum(struct save *save)
{
    Uint16 crc = 0xffff;
    int i;
    int bit;

    for (i = sizeof(save->checksum); (Uint32)i < sizeof(*save); i++) {
        crc ^= ((Uint8 *)save)[i];
        for (bit = 0; bit < 8; bit++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ CRC_POLYNOMIAL;
            } else {
                crc >>= 1;
            }
        }
    }
    crc ^= 0xffff;
    return crc;
}

Bool save_check_all(struct save *save)
{
    int i;

    for (i = 0; i < SAVE_COUNT; i++) {
        Uint16 expected = save->checksum;
        Uint16 actual = save_checksum(save);

        /* @hack redundant: save++ in an else after the return, which loads the
         * size of a save into R2 (0x8c08d882); after the if, into R3, and 2
         * of 29 words differ */
        if (actual != expected) {
            return FALSE;
        } else {
            save++;
        }
    }
    return TRUE;
}

static void pack_event_flags(struct save *save)
{
    int i;
    int byte;
    int bit;

    for (i = 0; i < FLAG_COUNT; i++) {
        if (i % 8 == 0) {
            save->event_flags[i / 8] = 0;
        }
        if (event_flags[i] == 1) {
            byte = i / 8;
            bit = 1 << (i % 8);
            save->event_flags[byte] |= bit;
        }
    }
}

static void pack_events_played(struct save *save)
{
    int i;
    int byte;
    int bit;

    for (i = 0; i < EVENT_COUNT; i++) {
        if (i % 8 == 0) {
            save->events_played[i / 8] = 0;
        }
        if (events_played[i] == 1) {
            byte = i / 8;
            bit = 1 << (i % 8);
            save->events_played[byte] |= bit;
        }
    }
}

static void store_unk_2e0(struct save *save)
{
    int i;

    for (i = 0; i < CHARACTER_COUNT; i++) {
        save->unk_2e0[i] = dat_8c7a2718[i];
    }
}

static void store_stage_clears(struct save *save)
{
    int character;
    int stage;

    for (character = 0; character < CHARACTER_COUNT; character++) {
        for (stage = 0; stage < STAGE_COUNT; stage++) {
            save->stage_clears[character][stage] =
                stage_clears[character][stage];
        }
    }
}

/* copy the story records into save field by field, leaving its unk_01s as
 * they were. load_story copies them back the same way */
static void store_story(struct save *save)
{
    int i;

    for (i = 0; i < CHARACTER_COUNT; i++) {
        save->story_records[i].time = story_records[i].time;
        save->story_records[i].seq = story_records[i].seq;
        save->story_records[i].section = story_records[i].section;
        save->story_records[i].entrance = story_records[i].entrance;
        save->story_records[i].stage_act = story_records[i].stage_act;
        save->story_records[i].wait_at = story_records[i].wait_at;
    }
}

static void unpack_event_flags(struct save *save)
{
    int i;
    int bits;

#ifdef GCC
    /* GCC can't see that the first pass, at i 0, sets bits before the
     * test reads it */
    bits = 0;
#endif
    for (i = 0; i < FLAG_COUNT; i++) {
        if (i % 8 == 0) {
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            bits = save->event_flags[i / 8];
        }
        if (bits & 1) {
            event_flags[i] = 1;
        } else {
            event_flags[i] = 0;
        }
        bits >>= 1;
    }
}

static void unpack_events_played(struct save *save)
{
    int i;
    int bits;

#ifdef GCC
    /* GCC can't see that the first pass sets bits, as in
     * unpack_event_flags */
    bits = 0;
#endif
    for (i = 0; i < EVENT_COUNT; i++) {
        if (i % 8 == 0) {
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            bits = save->events_played[i / 8];
        }
        if (bits & 1) {
            events_played[i] = 1;
        } else {
            events_played[i] = 0;
        }
        bits >>= 1;
    }
}

static void load_unk_2e0(struct save *save)
{
    int i;

    for (i = 0; i < CHARACTER_COUNT; i++) {
        dat_8c7a2718[i] = save->unk_2e0[i];
    }
}

static void load_stage_clears(struct save *save)
{
    int character;
    int stage;

    for (character = 0; character < CHARACTER_COUNT; character++) {
        for (stage = 0; stage < STAGE_COUNT; stage++) {
            stage_clears[character][stage] =
                save->stage_clears[character][stage];
        }
    }
}

static void load_story(struct save *save)
{
    int i;

    for (i = 0; i < CHARACTER_COUNT; i++) {
        story_records[i].time = save->story_records[i].time;
        story_records[i].seq = save->story_records[i].seq;
        story_records[i].section = save->story_records[i].section;
        story_records[i].entrance = save->story_records[i].entrance;
        story_records[i].stage_act = save->story_records[i].stage_act;
        story_records[i].wait_at = save->story_records[i].wait_at;
    }
}

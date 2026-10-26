/* the menus' reads of the save in play: its messages option, its stories'
 * stages, its play time and its best records */
#include <sg_xpt.h>

#include "advertise/records.h"
#include "advertise/trial.h"
#include "advertise/wheel.h"
#include "player/player.h"
#include "progress/progress.h"
#include "save/card.h"
#include "save/save.h"
#include "stage/stage.h"

/* the steps of a minute, at 60 a second */
#define MINUTE_STEPS 3600

/* the slot of each story's character in cart_times and trial_times, which
 * hold the six in the order of their CHARACTER_ numbers, E-102 before Big */
static const Sint8 story_time_slots[] = {0, 1, 2, 3, 5, 4};

struct save *record_save(void)
{
    return &saves[save_slot];
}

int record_voice_only(void)
{
    struct save *save = record_save();

    return (save->flags & SAVE_NO_MESSAGES) >> 1;
}

void record_set_voice_only(Sint8 voice_only)
{
    struct save *save = record_save();

    if (voice_only) {
        save->flags |= SAVE_NO_MESSAGES;
    } else {
        /* @hack type: the mask cast to Uint8. SHC then loads it from the pool
         * as 0x00fd (MOV.W at 0x8c901ab8). uncast, it is MOV #-3, the pool
         * that record_save, record_voice_only and this function share loses a
         * word, and 3 of 24 words differ */
        save->flags &= (Uint8)~SAVE_NO_MESSAGES;
    }
}

int record_story(void)
{
    struct save *save = record_save();
    int story;

    switch (save->character) {
    case CHARACTER_TAILS:
        story = STORY_TAILS;
        break;
    case CHARACTER_KNUCKLES:
        story = STORY_KNUCKLES;
        break;
    case CHARACTER_AMY:
        story = STORY_AMY;
        break;
    case CHARACTER_BIG:
        story = STORY_BIG;
        break;
    case CHARACTER_E102:
        story = STORY_E102;
        break;
    default:
        story = STORY_SONIC;
        break;
    }
    return story;
}

int record_story_stage(int story)
{
    struct save *save = record_save();
    struct story_record *record = &save->story_records[story_characters[story]];
    /* @hack spelling: stage_act, a copy of the record's. SHC then copies the
     * word to R4 (MOV R0,R4 at 0x8c901ba2) before it zero-extends it. read
     * from the record, it is zero-extended in R0, and 6 of 31 words differ */
    Uint16 stage_act = record->stage_act;

    /* @hack redundant: STAGE_OF's mask of the high byte, which the shift alone
     * would keep. SHC then ANDs with the mask (AND R3,R0 at 0x8c901ba6), and
     * shifts by SHAD (0x8c901ba8). without it, a SHLR8 shifts, the function
     * runs 6 bytes shorter, and 9 of 31 words differ */
    return STAGE_OF(stage_act);
}

Uint16 record_stage(void)
{
    struct save *save = record_save();
    /* @hack spelling: stage, a copy of the save's. SHC then holds the save's
     * address in R4 and the field's offset in R0 (MOV.W @(R0,R4),R0 at
     * 0x8c901bc6). returned directly, the two take R0 and R1, and 3 of 15
     * words differ */
    Uint16 stage = save->stage;

    return stage;
}

int record_play_minutes(void)
{
    struct save *save = record_save();
    /* @hack spelling: time, a copy of the save's. SHC then holds the save's
     * address in R4 (MOV.L @(4,R4),R1 at 0x8c901c1e). divided directly, it
     * holds it in R1, and 2 of 20 words differ */
    int time = save->play_time;

    return time / MINUTE_STEPS;
}

Uint8 record_stage_index(Uint8 stage, const struct trial_stages *list)
{
    Uint8 i;

    for (i = 0; i < list->count; i++) {
        if (list->stages[i].stage == stage) {
            break;
        }
    }
    return i;
}

/* where the story's records start in stage_scores, stage_times and stage_rings.
 * they hold each story's trial stages in turn, in the order of the stories
 * but with E-102's before Big's */
static int record_stage_offset(int story)
{
    int offset = 0;

    switch (story) {
    case STORY_BIG:
        offset += trial_stage_lists[STORY_E102].count;
        /* fall through */
    case STORY_E102:
        offset += trial_stage_lists[STORY_AMY].count;
        /* fall through */
    case STORY_AMY:
        offset += trial_stage_lists[STORY_KNUCKLES].count;
        /* fall through */
    case STORY_KNUCKLES:
        offset += trial_stage_lists[STORY_TAILS].count;
        /* fall through */
    case STORY_TAILS:
        offset += trial_stage_lists[STORY_SONIC].count;
    }
    return offset;
}

/* @hack type: record_stage_score defined old-style. a call then widens the
 * stage to an int (EXTU.B at 0x8c909aba in trial_stage_select_draw_score),
 * and the function narrows it to a Uint8 again (MOV.B R0,@(8,R15) at
 * 0x8c901cd6). with a prototype, the call passes the byte as MOV.B loads it,
 * and 21 of the caller's 34 words differ */
/* clang-format off */
/* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
int record_stage_score(story, stage)
    int story;
    Uint8 stage;
/* clang-format on */
{
    struct save *save = record_save();
    int index = record_stage_index(stage, &trial_stage_lists[story]);
    /* @hack spelling: score, which holds the result for the return. SHC then
     * builds the item's address in R4 and loads through it (MOV.L @R4,R0 at
     * 0x8c901d08). returned directly, the item is loaded indexed, and 11 of 37
     * words differ */
    int score;

    index += record_stage_offset(story);
    score = save->stage_scores[index];
    return score;
}

/* @hack type: record_fishing_score defined old-style. a call then widens
 * the stage to an int (EXTU.B at 0x8c909b9e in
 * trial_stage_select_draw_time). with a prototype, 39 of the caller's 83
 * words differ */
/* clang-format off */
/* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
int record_fishing_score(stage)
    Uint8 stage;
/* clang-format on */
{
    struct save *save = record_save();
    int index = record_stage_index(stage, &trial_stage_lists[STORY_BIG]);
    /* @hack spelling: score, which holds the result for the return. SHC then
     * loads the score into R1 (MOV.W @R4,R1 at 0x8c901d42), and multiplies it
     * there. returned directly, the score is loaded indexed into R0, and 12 of
     * 33 words differ */
    int score = save->fishing_scores[index][0] * 10;

    return score;
}

/* @hack type: record_stage_time defined old-style. a call then widens the
 * stage to an int (EXTU.B at 0x8c909b70 in trial_stage_select_draw_time),
 * and the function narrows it to a Uint8 again (MOV.B R0,@(12,R15) at
 * 0x8c901d90). with a prototype, 60 of the caller's 83 words differ */
/* clang-format off */
/* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
void record_stage_time(time, story, stage)
    struct save_time *time;
    int story;
    Uint8 stage;
/* clang-format on */
{
    struct save *save = record_save();
    int index = record_stage_index(stage, &trial_stage_lists[story]);

    index += record_stage_offset(story);
    *time = save->stage_times[index];
}

/* @hack type: record_stage_rings defined old-style. a call then widens the
 * stage to an int (EXTU.B at 0x8c909c5c in trial_stage_select_draw_rings),
 * and the function narrows it to a Uint8 again (MOV.B R0,@(8,R15) at
 * 0x8c901dee). with a prototype, the functions after the caller move, and 23
 * of the caller's 37 words differ */
/* clang-format off */
/* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
Sint16 record_stage_rings(story, stage)
    int story;
    Uint8 stage;
/* clang-format on */
{
    struct save *save = record_save();
    int index = record_stage_index(stage, &trial_stage_lists[story]);
    /* @hack spelling: rings, which holds the result for the return. SHC then
     * builds the item's address in R4 and loads through it (MOV.W @R4,R0 at
     * 0x8c901e22). returned directly, the item is loaded indexed, and 9 of 38
     * words differ */
    Sint16 rings;

    index += record_stage_offset(story);
    rings = save->stage_rings[index];
    return rings;
}

enum record_kind record_kind(int story, Uint8 stage)
{
    enum record_kind kind = RECORD_SCORE;

    if (stage == STAGE_CART) {
        kind = RECORD_CART_TIME;
    }
    switch (story) {
    case STORY_SONIC:
        if (stage == STAGE_EGGMOBILE3) {
            kind = RECORD_TRIAL_TIME;
        }
        break;
    case STORY_TAILS:
        if (stage == STAGE_EGGMOBILE2) {
            kind = RECORD_TRIAL_TIME;
        }
        break;
    case STORY_KNUCKLES:
        if (stage == STAGE_CHAOS6) {
            kind = RECORD_TRIAL_TIME;
        }
        break;
    case STORY_AMY:
        if (stage == STAGE_EGGMANROBO) {
            kind = RECORD_TRIAL_TIME;
        }
        break;
    case STORY_BIG:
        if (stage == STAGE_CHAOS6) {
            kind = RECORD_TRIAL_TIME;
        }
        break;
    case STORY_E102:
        if (stage == STAGE_E101_R) {
            kind = RECORD_TRIAL_TIME;
        }
        break;
    }
    return kind;
}

void record_cart_time(struct save_time *time, int story, int stage, int rank)
{
    struct save *save = record_save();
    /* @hack spelling: slot, a copy of the story's entry in story_time_slots.
     * SHC then adds cart_times' offset to the save's address before it reads
     * the entry (ADD R1,R3 at 0x8c901ee2). indexed by the table itself, it adds
     * the offset after, and 22 of 34 words differ */
    Sint8 slot = story_time_slots[story];

    *time = save->cart_times[slot].time[rank];
}

void record_trial_time(struct save_time *time, int story, int stage, int rank)
{
    struct save *save = record_save();
    /* @hack spelling: slot, a copy of the story's entry in story_time_slots.
     * SHC then loads trial_times' offset (MOV.W at 0x8c901f28) before it scales
     * the slot. indexed by the table itself, it loads the offset after, and 16
     * of 35 words differ */
    Sint8 slot = story_time_slots[story];

    *time = save->trial_times[slot].time[rank];
}

int record_score(int story, Uint8 stage, int rank)
{
    struct save *save = record_save();

    switch (stage) {
    case STAGE_SHOOTING1:
        story = save->shooting1_scores[story].score[rank];
        break;
    case STAGE_SHOOTING2:
        story = save->shooting2_scores[story].score[rank];
        break;
    case STAGE_ICECAP:
        story = save->icecap_scores[story].score[rank];
        break;
    case STAGE_SUNABOARD:
        story = save->sunaboard_scores[story].score[rank];
        break;
    case STAGE_PRACTICE:
        story = save->mole_scores.score[rank];
        break;
    }
    return story;
}

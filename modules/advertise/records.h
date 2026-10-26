/* the menus' reads of the save in play: its messages option, its stories'
 * stages, its play time and its best records */
#ifndef ADVERTISE_RECORDS_H
#define ADVERTISE_RECORDS_H

#include <sg_xpt.h>

#include "advertise/trial.h"
#include "save/save.h"

/* what a mini game records: scores, the cart race's times in hundredths, or the
 * boss trials' times */
enum record_kind { RECORD_SCORE, RECORD_CART_TIME, RECORD_TRIAL_TIME };

/* the save in play, saves[save_slot] */
struct save *record_save(void);
/* record_voice_only returns 1 when the save's events play their voices with no
 * messages, and 0 when they show both. record_set_voice_only sets the option
 * for a nonzero voice_only, and clears it for 0 */
int record_voice_only(void);
void record_set_voice_only(Sint8 voice_only);
/* the story of the character the save last played as, as enum wheel_story
 * numbers it. a character with no story of its own gives STORY_SONIC */
int record_story(void);
/* the stage story goes on from in the save in play: the high byte of the
 * stage_act of its character's story_record */
int record_story_stage(int story);
/* the save's stage_last_cleared: the stage of the last clear it counts.
 * record_play_minutes returns the save's play time in whole minutes */
Uint16 record_stage(void);
int record_play_minutes(void);
/* the index of stage in list->stages, or list->count when it isn't there */
Uint8 record_stage_index(Uint8 stage, const struct trial_stages *list);
/* record_stage_score, record_stage_time and record_stage_rings give the save's
 * best score, best time and most rings for the story on a stage of its trial
 * list. record_fishing_score gives Big's best fishing score on a stage of his
 * list. it is ten times the figure the save keeps, and the action stage list
 * shows it as a weight in grams.
 *
 * none of the four has a prototype. record_stage_score and record_stage_rings
 * take (int story, Uint8 stage), and record_fishing_score (Uint8 stage).
 * record_stage_time takes (struct save_time *time, int story, Uint8 stage), and
 * copies the time into *time */
/* NOLINTBEGIN(*-strict-prototypes,*-deprecated-non-prototype) */
int record_stage_score();
int record_fishing_score();
void record_stage_time();
Sint16 record_stage_rings();
/* NOLINTEND(*-strict-prototypes,*-deprecated-non-prototype) */
/* what the story's records on a mini game's stage are: RECORD_CART_TIME for the
 * cart, RECORD_TRIAL_TIME for the story's boss, and RECORD_SCORE for any
 * other */
enum record_kind record_kind(int story, Uint8 stage);
/* record_score returns the save's score at rank, 0 for the best, below
 * SAVE_BEST. it reads Sky Chase's two acts and Ice Cap's and Sand Hill's
 * boards, for which story must be STORY_SONIC or STORY_TAILS. it reads the
 * Hedgehog Hammer for any story. for any other stage it returns story
 * unchanged.
 * record_cart_time and record_trial_time copy the save's time at rank into
 * *time. their story is below STORY_LAST, and they ignore stage */
int record_score(int story, Uint8 stage, int rank);
void record_cart_time(struct save_time *time, int story, int stage, int rank);
void record_trial_time(struct save_time *time, int story, int stage, int rank);

#endif

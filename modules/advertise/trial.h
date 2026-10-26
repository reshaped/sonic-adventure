/* the trial's lists, which stories, stages and choices the menus open, and
 * what the menus hand the game as they close */
#ifndef ADVERTISE_TRIAL_H
#define ADVERTISE_TRIAL_H

#include <sg_xpt.h>

/* an entry of a trial list: a stage, and the act the trial plays there */
struct trial_stage {
    Uint8 stage;
    Uint8 act;
};

/* a trial list: count entries, from stages */
struct trial_stages {
    const struct trial_stage *stages;
    Uint8 count;
};

/* the 16 bytes each voice file saves as its data, "Team Andromeda\r\n" with no
 * NUL */
extern const char voice_save_data[16];
/* the stage and act a trial list picked, which advertise_apply hands the game.
 * trial_reset clears it */
extern struct trial_stage trial_choice;
/* each stage's name, by stage. file_stage_name_numbers gives the file
 * screen's, as a texture of ava_stnam. trial_stage_name_numbers gives the
 * trial lists', as an index into stage_name_textures, and gives every boss
 * the same one */
extern const Uint8 file_stage_name_numbers[];
extern const Uint8 trial_stage_name_numbers[];
/* by enum wheel_story: each story's character, CHARACTER_SONIC for the last
 * story, and the action stages of each story below STORY_LAST. the save keeps
 * a story's stage records in its list's order */
extern const Uint16 story_characters[];
extern const struct trial_stages trial_stage_lists[];
/* the entries of the longest trial list, Sonic's action stages */
#define LONGEST_TRIAL_LIST 10

/* clear trial_choice */
void trial_reset(void);
/* the number of the story's action stages, or of its mini games, that the
 * trial can play. those are the ones its character has cleared, as stage_clears
 * counts, or all of them while menu_state.all_open is set. story must be below
 * STORY_LAST */
Uint8 trial_stage_count(int story);
Uint8 trial_mini_game_count(int story);
/* the sums of trial_stage_count and of trial_mini_game_count over the stories
 * below STORY_LAST */
Uint8 trial_stage_total(void);
Uint8 trial_mini_game_total(void);
/* copy into list->stages, in their list's order, the action stages or mini
 * games that the trial can play for menu_state.story. list->count gets their
 * number. list->stages must point to writable room for the story's whole
 * list */
void trial_stage_list(struct trial_stages *list);
void trial_mini_game_list(struct trial_stages *list);
/* character_playable returns 1 once the story's character is playable, which
 * for the last story is once the six others have ended, and 0 before.
 * story_cleared returns TRUE once the story has ended. both hold for every
 * story while menu_state.all_open is set. story must be below STORIES */
int character_playable(int story);
Bool story_cleared(int story);
/* TRUE when the flash memory holds the browser's internet settings, as
 * save_isp_found finds, or while menu_state.all_open is set */
Bool internet_open(void);
/* hand the game the menus' choices. it sets messages_shown from the save's
 * messages option, last_story from menu_state.story, and the character number
 * to the story's character. it makes trial_choice the current stage and act.
 * in the mini game trial, though, four stories' bosses start at another boss
 * than the one trial_choice names, at act 0. it then releases every
 * character's textures */
void advertise_apply(void);
/* advertise_save_reset starts a new game with progress_init, then calls
 * save_reset_all. advertise_save_reset_write calls save_reset_all_write.
 * advertise_save_write, advertise_save_read, advertise_save_file_exists and
 * advertise_save_load each call the save_ function their name ends with.
 * each keeps its function's contract, but
 * advertise_save_reset_write and advertise_save_read drop the result */
void advertise_save_reset(void);
void advertise_save_reset_write(void);
int advertise_save_write(void);
void advertise_save_read(void);
Bool advertise_save_file_exists(void);
void advertise_save_load(void);

#endif

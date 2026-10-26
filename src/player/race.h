/* Tails' race against Sonic or Eggman, its progress bar and its result
 * banner */
#ifndef PLAYER_RACE_H
#define PLAYER_RACE_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

enum race_outcome { RACE_RUNNING, RACE_WON, RACE_LOST };

/* a point of a course. attr's low byte is the point's kind. the bits above
 * it hold flags, a wait and a stick way. race.c defines each, and what
 * Sonic does by it */
struct race_point {
    Sint32 attr;
    NJS_POINT3 pos;
};

/* the MILESRACE textures. Speed Highway's and Sky Deck's pvm lists load
 * them. so do the pvm lists of the Windy Valley, Ice Cap and Casinopolis
 * modules */
extern NJS_TEXLIST milesrace_texlist;

/* start Tails' race, in an act that has one, as the stage makes its
 * players. it does nothing unless character is CHARACTER_TAILS and player
 * is 1. for Tails it sets the outcome to RACE_RUNNING in every act. on
 * Speed Highway, the race task is Eggman. in the other races, it makes
 * Sonic player 1 and drives his pad. Ice Cap has no race while the trial
 * menu plays it as a mini game */
void race_create(Uint16 character, Sint16 player, Sint16 stage);
void race_set_course(Sint32 course);
/* the outcome of Tails' race. on Speed Highway, Eggman's race task sets
 * it. on Sky Deck, Tails' clear sets it, lost if Sonic is on the capsule's
 * button. in Windy Valley, Ice Cap and Casinopolis, the emerald sets it as
 * Tails or Sonic reaches it. a clear in a stage with no race sets
 * RACE_WON. race_result_step is the banner that shows it */
void race_set_won(void);
void race_set_lost(void);
enum race_outcome race_outcome(void);
void race_result_step(struct task *task);
/* the first steps of tasks that load the race's textures, MILESRACE and
 * Eggman's MRACE_EGGMOBLE. each task then idles, and releases its textures
 * as it's destroyed. Speed Highway's module makes Eggman's in Tails' first
 * act */
void race_texture_step(struct task *task);
void race_eggman_texture_step(struct task *task);

#endif

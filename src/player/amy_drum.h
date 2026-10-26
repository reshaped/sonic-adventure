/* the drums Amy takes up and sets down */
#ifndef PLAYER_AMY_DRUM_H
#define PLAYER_AMY_DRUM_H
#include <ninja.h>

struct task;

/* create a drum at a copy of *pos. TRUE when it's created, or FALSE when no
 * task can be created */
Bool amy_drum_create(NJS_POINT3 *pos);
/* create the drums the current act sets out, if the game's character is
 * Amy. TRUE once it creates them all, and then it forgets the touched drum.
 * FALSE for another character or an act with no drums, or when a drum's
 * task can't be created, leaving the drums before it. FALSE keeps the
 * touched drum */
Bool amy_drum_create_all(void);
/* the drum a player last touched while facing it, or null. the touched drum
 * is forgotten by amy_drum_clear_touched, by amy_drum_create_all when it
 * succeeds, and at the end of each of Amy's steps */
struct task *amy_drum_get_touched(void);
/* forget the touched drum: amy_drum_get_touched returns null until a player
 * touches one again */
void amy_drum_clear_touched(void);

#endif

/* CRACKER, a party cracker that fires confetti, and streamers too, while the
 * crackers are on */
#ifndef AL_RACE_CRACKER_H
#define AL_RACE_CRACKER_H
#include <sg_xpt.h>

#include "system/task.h"

/* CRACKER's step in the race's object kinds. it sets the cracker's own
 * step, draw and destructor, except while the set editor holds the task.
 * there this stays the step. each step it labels the scale's rows and draws
 * the cracker, and every 60 steps it fires confetti for the editor */
void cracker_init(struct task *task);
/* turn the crackers off */
void cracker_stop(void);
/* turn the crackers on. they fire streamers and confetti when no rival won.
 * they fire confetti alone when a rival won but the second or third place
 * is no rival's, and nothing otherwise */
void cracker_start(void);
/* TRUE while the crackers are on */
Bool cracker_on(void);

#endif

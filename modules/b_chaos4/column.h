/* Chaos 4's columns of water, the wakes he leaves as he swims, and their
 * counts */
#ifndef B_CHAOS4_COLUMN_H
#define B_CHAOS4_COLUMN_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* make a column of water that bursts up out of the swamp's surface at
 * pos's x and z. it moves by speed each step, or stands still for a null
 * speed. it makes none while 5 or more stand, or when it can't allocate its
 * task or its record */
void column_create(NJS_POINT3 *pos, NJS_VECTOR *speed);
/* 1 while player 0 is Tails flying or Knuckles gliding, whom a column
 * brings down, else 0 */
int chaos4_player_flying(void);
/* make a wake, a ring that spreads out on the swamp's surface at pos's x
 * and z, across ang's heading. it makes none while more than 20 spread, or
 * when it can't allocate its task */
void wake_create(NJS_POINT3 *pos, struct angles *ang);

/* the counts of the columns standing and the wakes spreading. their
 * tasks count up as they start and down as they end, and the stage clears
 * both as it starts */
void effect_counts_clear(void);
void column_count_add(void);
void column_count_remove(void);
int column_count(void);
void wake_count_add(void);
void wake_count_remove(void);
int wake_count(void);

/* the columns' textures and the wakes', which the stage's pvm list loads
 * from CHAOS4_HASHIRA and CHAOS4_WAVE */
extern NJS_TEXLIST column_texlist;
extern NJS_TEXLIST wake_texlist;

#endif

/* the sky deck's wind, and three of the deck's objects: O_VENT, a vane the
 * wind swings; O_DECAL, a scrap of deck; and O_SIGNAL, a lamp */
#ifndef STG06_WIND_H
#define STG06_WIND_H
#include <sg_xpt.h>

#include "system/task.h"

/* the step of the wind's task, which the first act starts. its first step
 * sets every band's life to 0, and frees every gust. each later step moves
 * the bands. it starts a free one at least 20 steps after the last start. a
 * band is free on every other step once its life has run out */
void sky_deck_wind_step(struct task *task);
/* give task a gust 150 in reach round its x and z, or move its gust there.
 * the table has 32 entries, and the call takes the first that is free or
 * task's. so a free entry ahead of task's gust becomes a new one. the old
 * one stays where it was. while every entry is another task's, nothing
 * changes */
void sky_deck_wind_gust_set(struct task *task);
/* free the first entry that is task's. when task has two gusts, the older,
 * later in the table, outlives task. a later task at the same address then
 * counts as its owner. the gust blows where it was until such a task moves
 * or frees it, or the wind's task starts again */
void sky_deck_wind_gust_remove(struct task *task);
/* how hard the wind blows at x and z, from 0 to 256 */
Float sky_deck_wind_strength(Float x, Float z);
/* add the wind's lift at player 0 to its accel.y, then cap accel.y at the
 * lift */
void sky_deck_wind_lift_player(void);
/* the steps sky_deck_object_list names for O_VENT, O_DECAL and O_SIGNAL */
void sky_deck_vent_step(struct task *task);
void decal_step(struct task *task);
void signal_step(struct task *task);

#endif

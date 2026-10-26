/* each character's tutorial: its pages, and the page task, which shows one
 * page at a time in a box and turns from one to another. a tutorial's
 * first pages are its controls pages, and the rest are its game pages.
 * tutorial_screen.c shows each kind over the back's page of that name */
#ifndef SUMMARY_TUTORIAL_H
#define SUMMARY_TUTORIAL_H

#include <ninja.h>
#include <sg_xpt.h>

#include "draw/screen.h"
#include "system/task.h"

/* the stories with a tutorial: the six characters', not the last story */
#define TUTORIALS 6

/* the page task's modes, in its work->mode. TUTORIAL_SHOWN holds the page
 * shown open. TUTORIAL_TURN shrinks it to no height, a tenth of it a step,
 * then shows the next page and moves to TUTORIAL_OPEN. TUTORIAL_OPEN grows
 * that page back open a tenth a step, then moves to TUTORIAL_SHOWN */
enum tutorial_mode { TUTORIAL_SHOWN, TUTORIAL_OPEN, TUTORIAL_TURN };

/* a bit of the page task's work->flags. it is set for one step, the step on
 * which the page shown crosses between the controls pages and the game
 * pages */
#define TUTORIAL_CROSSED 0x2

/* what the last tutorial_find_page found. TUTORIAL_FIRST and TUTORIAL_LAST
 * mean it found no page to show before or after the page shown */
enum tutorial_edge { TUTORIAL_INSIDE, TUTORIAL_FIRST, TUTORIAL_LAST };

/* load story's tutorial textures from their PVMs, and release them. story
 * must be a character's, from 0 to 5, unchecked */
void tutorial_load_textures(int story);
void tutorial_release_textures(int story);
/* draw picture as screen_parts_draw does, at depth z. each part and its
 * offset from x and y are scaled by scale_x and scale_y. unused is ignored */
void tutorial_parts_draw(struct screen_parts *picture, int unused, Float x,
                         Float y, Float z, Float scale_x, Float scale_y);
/* create the page task in level for story's tutorial, and return it, or
 * null when task_create fails. story must be a character's, from 0 to 5,
 * unchecked. the caller loads its textures with tutorial_load_textures. the
 * task shows the first page open at pos, or with pos null at (200, 96,
 * 10000). it keeps the tutorial in its work's vars.p[2]. its record's
 * allocation goes unchecked */
struct task *tutorial_create(int level, int story, NJS_POINT3 *pos);
/* the nearest page that shows past the page shown: after it for direction
 * 1, or before it for -1. a page shows while its event flag is set, and a
 * page without one always shows. the edge becomes TUTORIAL_INSIDE. with no
 * such page, the result is the page shown, and the edge TUTORIAL_LAST or
 * TUTORIAL_FIRST. any other direction gives the page shown, with the edge
 * TUTORIAL_INSIDE. each call zeroes the work's id */
int tutorial_find_page(struct task *task, int direction);
/* how many of the tutorial's pages, from the first, are controls pages */
int tutorial_controls(struct task *task);
/* the page shown */
int tutorial_get_page(struct task *task);
/* show page at once, without a turn */
void tutorial_set_page(struct task *task, int page);
/* set the page a turn shows once the page shown has closed */
void tutorial_set_next(struct task *task, int page);
/* the task's mode, an enum tutorial_mode */
int tutorial_get_mode(struct task *task);
void tutorial_set_mode(struct task *task, int mode);
enum tutorial_edge tutorial_get_edge(struct task *task);
/* turn to page: set the next page to page and the mode to TUTORIAL_TURN */
void tutorial_turn(struct task *task, int page);

#endif

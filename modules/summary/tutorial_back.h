/* the tutorial's back: a task that draws one of a character's two pages,
 * with its title, behind the box the page task draws. a turn fades the
 * other page in over it */
#ifndef SUMMARY_TUTORIAL_BACK_H
#define SUMMARY_TUTORIAL_BACK_H

#include <ninja.h>
#include <sg_xpt.h>

#include "draw/screen.h"
#include "system/task.h"

/* a back's pages. tutorial_screen.c shows the tutorial's controls pages
 * over BACK_CONTROLS, and its game pages over BACK_GAME */
enum { BACK_CONTROLS, BACK_GAME };

/* a back's modes, in its work->mode. BACK_STILL shows the page shown
 * alone. BACK_TURN_START is a turn's first step, which sets blend to 0 and
 * moves to BACK_TURNING. BACK_TURNING fades the page shown in over the page
 * before it, then moves to BACK_STILL */
enum { BACK_STILL, BACK_TURN_START, BACK_TURNING };

/* the back task's record, in task->extra */
struct tutorial_back {
    int unk_00[2];
    const struct screen_parts *pages[2]; /* each page's picture, by page */
    Uint32 color; /* multiplies both pages and titles during a turn */
    NJS_TEXLIST *texlist; /* the texlist the pictures draw from */
    /* the alpha of the page shown and its title during a turn, from 0 to
     * 1. the page before it and its title take 1 - blend */
    Float blend;
    Float speed; /* added to blend each step of a turn */
    Uint8 shown; /* the page shown, a BACK_ page */
    Uint8 before; /* the page a turn fades out */
};

/* a color for each character's story. nothing reads them: a back's color
 * is white */
extern Uint32 tutorial_back_colors[];
/* the titles of both pages and the bar, which load from the tuto_cmn PVM */
extern NJS_TEXLIST tutorial_title_texlist;

/* make Ninja's back color, which fills the screen behind every draw,
 * opaque white. unused is ignored */
void tutorial_whiten_back_color(int unused);
/* load story's back PVM into its texlist, and return the texlist. story
 * must be a character's, from 0 to 5, unchecked */
NJS_TEXLIST *tutorial_back_load(int story);
/* create the back task in level for story's back, and return it, or null
 * when task_create fails. story must be a character's, from 0 to 5,
 * unchecked. the caller loads the back's texlist with tutorial_back_load,
 * and tutorial_title_texlist from the tuto_cmn PVM. the task shows the
 * controls page still. the top left of the page's picture is at pos, or
 * with pos null at (16, 176) and depth 20000. its record's allocation goes
 * unchecked */
struct task *tutorial_back_create(int level, int story, NJS_POINT3 *pos);
/* set the back's mode, a BACK_ mode */
void tutorial_back_set_mode(struct task *task, int mode);
/* set the mode to BACK_TURN_START, to fade from the record's before page to
 * its shown page. the caller sets both first */
void tutorial_back_turn(struct task *task);

#endif

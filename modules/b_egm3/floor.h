/* the floor the fight stands on: two rows of eight panels, each its own
 * task. the boss shatters them */
#ifndef B_EGM3_FLOOR_H
#define B_EGM3_FLOOR_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* row 0 lies from x -320 to -200, about x -FLOOR_ROW_X, and row 1 from 200
 * to 320, about FLOOR_ROW_X. each runs along z from -FLOOR_ROW_END_Z to
 * FLOOR_ROW_END_Z, FLOOR_PANEL_LENGTH to a panel, with column 0 at its low
 * end. its end panels are centered at z -FLOOR_LAST_PANEL_Z and
 * FLOOR_LAST_PANEL_Z */
#define FLOOR_ROWS 2
#define FLOOR_COLUMNS 8
#define FLOOR_ROW_X 260.0F
#define FLOOR_ROW_END_Z 480.0F
#define FLOOR_PANEL_LENGTH 120.0F
#define FLOOR_LAST_PANEL_Z 420.0F

/* FLOOR_PANEL and FLOOR_END_PANEL are the kinds of panel, and index
 * floor_panel_objects and floor_panel_solids. at FLOOR_SHARDS and
 * FLOOR_END_SHARDS, floor_panel_objects holds the trees each kind breaks
 * into. a row's first and last panels are end panels */
enum { FLOOR_PANEL, FLOOR_END_PANEL, FLOOR_SHARDS, FLOOR_END_SHARDS };

/* a panel task's extra, which its creator allocates and fills */
struct floor_panel {
    NJS_OBJECT *object; /* the tree it draws */
    /* its solid in the world. it is NULL when world_object_alloc had none to
     * lend, and once the panel's task is destroyed */
    NJS_OBJECT *solid;
    Uint8 unk_08[4];
    Float rise; /* how far it rises each step while it rises */
    Uint8 unk_10[16];
    Sint32 kind; /* FLOOR_PANEL or FLOOR_END_PANEL */
};

extern NJS_OBJECT *floor_panel_objects[4];
extern NJS_OBJECT *floor_panel_solids[2];
/* 1 while a panel stands, and 0 once it is told to shatter. SHC lays out
 * floor.c's globals in the order it first meets them. so floor_standing
 * stays declared ahead of floor_panels */
extern Sint8 floor_standing[FLOOR_ROWS][FLOOR_COLUMNS];
extern struct task *floor_panels[FLOOR_ROWS][FLOOR_COLUMNS];

/* a panel's step. its creator sets the task's place, turn, force and extra
 * before it runs */
void floor_panel_step(struct task *task);
/* floor_standing of the panel under x and z, and 0 off the floor. a z less
 * than FLOOR_PANEL_LENGTH below -FLOOR_ROW_END_Z, off the floor's low end,
 * reads as column 0 */
int floor_standing_at(Float x, Float z);
/* what floor_row_middle returns when no panel in the row stands */
#define FLOOR_NONE_STANDING 1000000.0F
/* the middle, in z, of a span of the row at x. the span runs from the low
 * end of the lowest standing panel to the middle of the highest. with the
 * whole row standing, the result is -30, a quarter panel below the row's
 * middle */
Float floor_row_middle(Float x);
/* tell the standing panel under x and z to rise 60, or to shatter. a panel
 * told to shatter stops standing at once. a panel that rises leaves its
 * solid where it was, and nothing in the image calls floor_panel_rise */
void floor_panel_rise(Float x, Float z);
void floor_panel_break(Float x, Float z);

#endif

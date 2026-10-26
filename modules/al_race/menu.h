/* the Chao race's menus. slots keep their icons and models, a panel stands
 * behind them, and the menu_draw_ functions draw without a task */
#ifndef AL_RACE_MENU_H
#define AL_RACE_MENU_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* how a slot's icon or model looks on its next step, as menu_slot_look sets
 * it. each step sets the look back to LOOK_PLAIN, but LOOK_HIDDEN holds, and
 * so does LOOK_POP on an icon */
enum menu_look {
    LOOK_PLAIN,
    LOOK_RED, /* an icon tinted red, and a model turned back to its start */
    LOOK_BACK, /* an icon drawn behind the rest */
    LOOK_YELLOW, /* an icon tinted yellow */
    LOOK_PULSE, /* pulsing: an icon drawn in front of the rest, a model spun */
    /* pulsing: an icon drawn in front and tinted yellow, a model not spun */
    LOOK_PULSE_YELLOW,
    LOOK_RAISED, /* at 1.25 times its size */
    LOOK_HIDDEN, /* not drawn, with its scale and turn left as they are */
    LOOK_GONE, /* destroyed, and its slot emptied */
    /* an icon opens out, shows until 60 steps have passed, fades and is
     * gone. LOOK_POP + 1 and LOOK_POP + 2 do the same */
    LOOK_POP
};

/* allocate the twelve slots, each empty, and return them, or null when
 * heap_calloc fails. without the slots, the menu_show_ functions and
 * menu_slot_look do nothing */
struct task **menu_slots_create(void);
/* destroy each slot's task, and free the slots */
void menu_slots_free(void);
/* 1 when there are slots and every one is empty, else 0 */
int menu_slots_empty(void);
/* destroy each slot's task, and empty the slot */
void menu_slots_clear(void);
/* give slot's icon or model look for its next step, and return 1. return 0
 * when there are no slots or slot is empty. slot must be 0 to 11,
 * unchecked */
int menu_slot_look(int slot, int look);

/* show the three race modes' icons in slots 0 to 2, back in 3 and the
 * menu's title in 4 */
void menu_show_modes(void);
/* show the five courses' models in slots 0 to 4, back in 5 and the menu's
 * title in 6. with wide set, the models stand in one row. without it, four
 * stand in a row and the fifth below them */
void menu_show_courses(int wide);
/* show the card drive's model in slot 0, the three gardens' pictures in 1
 * to 3, back in 4 and the menu's title in 5 */
void menu_show_gardens(void);
/* show start in slot 0, back in 1, the menu's title in 2 and the label of
 * the entries left in 3 */
void menu_show_entry(void);
/* show START, GOAL, COOL or GET in the middle of the screen, in slot 0 */
void menu_show_start(void);
void menu_show_goal(void);
void menu_show_cool(void);
void menu_show_get(void);

/* put up the panel behind the menus and return 1, or return 0 when it is up
 * already. the panel grows to its full size over MENU_PANEL_STEPS steps.
 * menu_panel_close takes it down */
#define MENU_PANEL_STEPS 30
int menu_panel_open(void);
void menu_panel_close(void);

/* the race's nine sprites, which menu_draw_sprite_at draws */
extern const NJS_TEXANIM menu_sprites[9];

/* the draws below make no task: each draws once, as it is called */

/* draw course's name, tinted red, for a course below 5, and nothing for
 * another. course must not be negative */
void menu_draw_course(int course);
/* draw the message STATE_VMS_ERROR_CHECK shows while GAME_NOT_SAVED is set:
 * icons 24 and 25, its two halves, side by side */
void menu_draw_message(void);
/* draw the card drive's model in each of the CARD_DRIVES places, in four
 * columns of two. then draw the a-life window's icons 26 to 31, four under
 * the columns and two beside the rows */
void menu_draw_drives(void);
/* the entry kind before or after kind in the garden window's order:
 * ENTRY_CARD, GARDEN_STATION_SQUARE, GARDEN_MYSTIC_RUINS and
 * GARDEN_EGG_CARRIER, then round again. any other kind counts as
 * GARDEN_EGG_CARRIER */
int menu_garden_before(int kind);
int menu_garden_after(int kind);
/* draw the entry kinds before and after kind at the sides, small: a
 * garden's picture, or the card drive's model. unless kind is ENTRY_CARD,
 * draw its name large and yellow, and its garden's picture faint and at the
 * back */
void menu_draw_gardens(int kind);
/* draw frame of the race's nine sprites at scale, centred on x and y */
void menu_draw_sprite_at(Float x, Float y, Float scale, int frame);

/* create a task that draws course's model at pos, spinning, bobbing and
 * glinting, until the act changes to ACT_HALL. a course outside 0 to 4
 * makes none */
void menu_spin_create(int course, NJS_POINT3 *pos);

#endif

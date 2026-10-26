/* ADVERTISE's entry points, which start, step and end the menus. the main
 * loop calls them through advertise_entries, the table at the module's
 * head */
#ifndef ADVERTISE_ADVERTISE_H
#define ADVERTISE_ADVERTISE_H

#include "main.h"

struct advertise {
    int (*start)(enum menu_entrance *entrance);
    int (*end)(void *unused);
    int (*step)(enum menu_entrance *entrance);
};

extern struct advertise advertise_entries;

/* advertise_start resets the tasks, the heap, the sound and the textures. it
 * starts the menus' card access with card_open, and starts the menus at
 * *entrance. an entrance other than MENU_FROM_SUMMARY and MENU_FROM_TRIAL
 * starts them from the top, with the wheel at its first story. it returns
 * OVERLAY_RUNNING.
 *
 * advertise_step runs a frame of the menus. it returns OVERLAY_RUNNING until
 * they close. it then returns what they closed on: OVERLAY_DONE, OVERLAY_BACK
 * or any menu_choice other than MENU_EXIT. on the pads' quit it ends every
 * task. it then returns MENU_EXIT from the card screen, and OVERLAY_RESET from
 * any other.
 *
 * advertise_end ends the menus, and starts the game's card access again with
 * card_close. it returns 0 */
int advertise_start(enum menu_entrance *entrance);
int advertise_end(void *unused);
int advertise_step(enum menu_entrance *entrance);

#endif

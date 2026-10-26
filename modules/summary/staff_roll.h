/* the staff roll: the entry points summary_entries gives the main loop */
#ifndef SUMMARY_STAFF_ROLL_H
#define SUMMARY_STAFF_ROLL_H

/* start the roll of the story menu_get_story gives. it first empties the
 * heap, the tasks and the textures, and paces each frame at one vsync and
 * one step. it returns 1 */
int staff_roll_start(void *unused);
/* step the roll once. once the credits and the last picture end, the roll
 * checks the save the game plays for the story's emblem. where the save
 * lacks it, the roll sets it, shows the emblem count and writes the saves.
 * return OVERLAY_RUNNING while the roll runs, OVERLAY_DONE on the step it
 * ends, and OVERLAY_RESET on the pads' quit, as pad_quit_pressed reports
 * it. both ends run staff_roll_end */
int staff_roll_step(void *unused);
/* stop the music, destroy every task and release every texture */
void staff_roll_end(void *unused);

#endif

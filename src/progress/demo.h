/* Sega logo and paired attract-mode demos */
#ifndef PROGRESS_DEMO_H
#define PROGRESS_DEMO_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

/* what logo_step returns as the logo ends: go on to ADVERTISE's menus, or
 * play movie 0 */
enum logo_result { LOGO_ADVERTISE = 1, LOGO_MOVIE = 3 };

/* demo_event for a demo that plays recorded pads */
#define DEMO_NO_EVENT (-1)

/* TRUE from a demo's start until boot_init or menu_begin */
extern Bool demo_playing;
/* the event the demo plays, or DEMO_NO_EVENT */
extern int demo_event;
/* -1 once the demo's event lets go of control, for the stage to end the
 * demo */
extern int demo_event_ended;
/* the prompt to press Start, over a demo or a title guide */
extern NJS_TEXLIST press_start_texlist;

/* start the logo. it resets the heap and the tasks, loads the logo's
 * textures and paces one vsync and one step to a frame. the title's fade
 * starts black, and the cover is clear */
void logo_start(void);
/* start the logo for the game's boot. demo_next goes back to the first
 * demo, save_slot to 0, and save_reset_all resets the saves */
void boot_init(void);
/* run a frame of the logo. return 0 while it shows. on the frame it ends,
 * return LOGO_MOVIE where the first or fourth pair of demos plays next, and
 * LOGO_ADVERTISE where another does. the pads' quit exits to the boot ROM */
int logo_step(void);
/* set the flags every demo plays with, and FLAG_E102_BOOSTER for a demo of
 * recorded pads, as demo_event says. it clears no flag */
void demo_set_flags(void);
/* set up demo number demo_next, and move demo_next on, from the last
 * back to the first. it calls card_init and starts a new game with the
 * demo's flags, stage, act and character. the hud hides, and recording_kind
 * is RECORDING_PLAY. a demo of recorded pads reads its character's
 * recording to DEMO_RECORDING */
void demo_advance(void);
/* ready the game for ADVERTISE's menus or the title. it resets the heap and
 * the tasks, ends any demo or recording, and sets the stage to
 * STAGE_PRACTICE. the progress becomes the save the game plays, from the
 * memory card where it reads and otherwise as saves holds it */
void menu_begin(void);
/* the game mode after a stage quits. a demo that plays to its end keeps
 * RECORDING_PLAY. under it, after a pair's first demo, start its second
 * with draw_off set and return GAME_MODE_FIELD or GAME_MODE_ACTION. under
 * it, after the second, start the logo and return GAME_MODE_LOGO. otherwise
 * return 0. a pair's first demo left with Start, which sets RECORDING_OFF,
 * skips its second */
int demo_next_mode(void);
/* a task's step: destroy the streams and start demo_event. the task then
 * waits for the event to let go of control, sets demo_event_ended to -1 and
 * kills itself */
void demo_event_start(struct task *task);

#endif

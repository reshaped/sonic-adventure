/* overlays: the mission card, a demo's press start, and the pause menu */
#ifndef HUD_OVERLAY_H
#define HUD_OVERLAY_H

struct task;

/* show the card of the next mission the character has to win in the
 * stage, for 120 steps in the lower middle of the screen. the order is C,
 * then B, then A, and no card shows once A's emblem is won. Eggman and
 * Tikal get Sonic's cards. only the action stages show one. none shows in
 * Ice Cap played as a minigame from the trial menu, nor for Tails in
 * Emerald Coast or Red Mountain */
void mission_card_create(void);
/* show the card as mission_card_create does, but for 60 steps in the
 * screen's middle, and on every action stage */
void mission_card_create_score(void);
/* do nothing. a stage clear calls it as it starts the score screen */
void overlay_stage_clear(void);
/* the step of a demo's press start, which draws it at the top right. its
 * alpha pulses once every 64 steps. press_start_destroy, its destructor,
 * releases press_start_texlist */
void press_start_step(struct task *task);
void press_start_destroy(struct task *task);

enum pause_choice { PAUSE_CONTINUE, PAUSE_RESTART, PAUSE_QUIT, PAUSE_CHOICES };
/* draw the pause menu, with the choice under the cursor larger and framed.
 * pause_draw offers all three choices. pause_draw_no_restart offers
 * continue as choice 0 and quit as 1. pause_draw_no_choices draws the
 * banner and the back alone */
void pause_draw(enum pause_choice choice);
void pause_draw_no_restart(int choice);
void pause_draw_no_choices(void);

#endif

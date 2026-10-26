/* the SEQ EDITOR, a program of the <SELECT PROGRAM> menu. its pages set the
 * event flags, the events played and the stages cleared. they go to a stage
 * or a section of the story, and show where player 0 is */
#ifndef DEBUG_SEQ_EDITOR_H
#define DEBUG_SEQ_EDITOR_H

/* seq_editor_init readies the editor, and seq_editor_exec runs a step of it.
 * the step after the menu exits returns -1 and leaves the editor as
 * seq_editor_init does. every other step returns 0, and hides the menu's
 * buttons from the game as editor_take_buttons does. those are the first pad's
 * A, B, X and Y, its triggers and Start */
void seq_editor_init(void);
int seq_editor_exec(void);

#endif

/* O EME P, a place where a piece of the master emerald can lie for
 * Knuckles to find, and the piece as he takes it. also the sprite that
 * blinks over a target */
#ifndef OBJECT_PIECE_H
#define OBJECT_PIECE_H
#include <ninja.h>

struct task;

/* the textures of a piece's glints, k_eme_kiran01 to 04 */
extern NJS_TEXLIST eme_kiran_texlist;

/* the object lists' step for O EME P. outside the set editor, a mark
 * records where Knuckles restarts once he takes its piece. the mark then
 * ends. a place where a piece can lie ends unless its piece is one
 * Knuckles has yet to take, which then lies there */
void piece_start(struct task *task);
/* the step of the piece Knuckles takes, which knuckles_take_piece starts.
 * the piece rises, then flies to above him and goes */
void piece_taken_step(struct task *task);
/* the first step of the sprite that blinks over a target */
void target_blink_start(struct task *task);

#endif

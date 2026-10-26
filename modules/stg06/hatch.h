/* the hatches O CHAMBR: nine hatches in the deck that open as it tilts. an
 * open hatch releases a piece, which slides with the tilt */
#ifndef STG06_HATCH_H
#define STG06_HATCH_H

#include "system/task.h"

/* the step the object list names for O CHAMBR. it makes its table's nine
 * hatches as children and steps them. for Knuckles, it selects one of the
 * first six at random, which opens but releases no piece. the walk to that
 * hatch doesn't test for null, so six hatches must have been made */
void hatches_step(struct task *task);
/* each of these steps runs one of hatch.c's pieces as an object of its own.
 * they run pieces 8 and 0 to 5. a hatch's own pieces run another step. nothing
 * in the image or 1ST_READ.BIN names these. a piece needs a parent hatch, whose
 * work it reads and marks as it moves, untested. only its start and its burst
 * test for a parent. it needs a TASK_EXTRA record too, for where it started,
 * which it never tests. it bursts once farther than 550 from the parent, or 650
 * for piece 8. in the set editor, each draws its piece. there, piece 5 reads
 * through a null model, unless a step outside the editor has set one */
void hatch_piece_8_step(struct task *task);
void hatch_piece_0_step(struct task *task);
void hatch_piece_1_step(struct task *task);
void hatch_piece_2_step(struct task *task);
void hatch_piece_3_step(struct task *task);
void hatch_piece_4_step(struct task *task);
void hatch_piece_5_step(struct task *task);

#endif

/* talks: what player 0 starts talking to by pressing grab near it */
#ifndef PLAYER_TALK_H
#define PLAYER_TALK_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

/* talk_add's flags: what player 0 must be for the talk to start */
#define TALK_LEVEL 0x2 /* less than 10 above or below it */
#define TALK_FACING 0x4 /* facing it, less than 40 degrees off */

/* create the task that starts a talk as player 0 presses grab, and empty
 * the list of talks. the task frees the list as it dies. null when
 * task_create fails */
struct task *talk_create_manager(void);
/* add id's talk to the list, or change the talk id has. TRUE when it does,
 * and FALSE when the heap has no room for a new talk.
 *
 * the talk keeps pos, ang and active as pointers, not copies. active must
 * not be null, and is TRUE while the talk goes on. a talk with a pos starts
 * only while player 0 is nearer to it in x and z than its reach. the reach
 * is radius plus player 0's radius, but no more than 100. a talk with an ang
 * starts only while it faces player 0, less than 80 degrees off. of the
 * talks that may start, the nearest does. a talk with a null pos is a
 * fallback: the first on the list starts when no talk with a pos may */
Bool talk_add(void *id, NJS_POINT3 *pos, Angle *ang, Bool *active, Float radius,
              Sint32 flags);
/* end the talk going on, if any, setting its active FALSE. the pads drive
 * the players again either way */
void talk_end(void);
/* take id's talk off the list, ending it if it's going on */
void talk_remove(void *id);

#endif

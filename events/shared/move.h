/* an event's moves. each move runs as a mover: a task that sets another
 * task's place once a step. most moves set that task's angles too. at most
 * EVENT_MOVERS run at once, and a call with no free slot does nothing. a
 * mover spends its first two steps starting. it then sets its target once a
 * step, steps times. on the next step it frees its slot and kills itself.
 * steps must be 1 or more. target and base must outlive the move, since
 * the mover uses both each step. a call reads or writes through null when
 * task_create or heap_alloc fails */
#ifndef EVENT_MOVE_H
#define EVENT_MOVE_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* how many moves can run at once */
#define EVENT_MOVERS 10

/* a mover's work mode. each kind of move has three modes. the calls named
 * beside the first set it, and it starts the move. the second sets target
 * each step, and the third ends the move */
enum mover_mode {
    MOVER_IDLE,
    MOVER_LINE_START, /* event_move */
    MOVER_LINE,
    MOVER_LINE_END,
    MOVER_TURN_START, /* event_move_turn, event_move_turn_degrees */
    MOVER_TURN,
    MOVER_TURN_END,
    MOVER_FOLLOW_START, /* event_follow */
    MOVER_FOLLOW,
    MOVER_FOLLOW_END,
    MOVER_FOLLOW_TURN_START, /* event_follow_turn */
    MOVER_FOLLOW_TURN,
    MOVER_FOLLOW_TURN_END,
    MOVER_FOLLOW_ANGLES_START, /* event_follow_angles, event_follow_facing */
    MOVER_FOLLOW_ANGLES,
    MOVER_FOLLOW_ANGLES_END,
    /* no call starts MOVER_FACING. its step sets only target's place. the
     * place lies distance out from the origin, along a vector turned by
     * base's angles plus turn. the step makes that vector from a local no
     * code writes, so the place is undefined. event_follow_facing sets
     * distance, but starts MOVER_FOLLOW_ANGLES */
    MOVER_FACING_START,
    MOVER_FACING,
    MOVER_FACING_END,
    MOVER_WELD0_START, /* event_follow_weld0 */
    MOVER_WELD0,
    MOVER_WELD0_END,
    MOVER_WELD1_START, /* event_follow_weld1 */
    MOVER_WELD1,
    MOVER_WELD1_END
};

/* a mover's move, from the heap, which the mover's destructor frees. step
 * counts the steps that have set target, up to steps */
struct event_move {
    struct task *target;
    struct task *base; /* the task a follow move tracks */
    /* from and to keep a line move's ends, and ang_to its turn's end. no
     * step reads them */
    NJS_POINT3 from;
    NJS_POINT3 to;
    NJS_VECTOR speed; /* a line move's change of place each step */
    NJS_POINT3 offset; /* from base's place, or added to the weld point */
    /* the angles MOVER_LINE and MOVER_FOLLOW hold target at */
    struct angles ang;
    struct angles ang_to;
    /* each step adds turn to the mover's angles, or to base's angles */
    struct angles turn;
    Sint32 player; /* the player whose weld point a weld move follows */
    Sint32 steps;
    Sint32 step;
    Sint32 slot; /* the mover's index in event_movers */
};

/* a mover's work vars. the work's pos and ang hold the place and angles the
 * mover gives target */
struct mover_vars {
    Uint8 unk_00[4];
    Float distance; /* how far MOVER_FACING sets target from the origin */
    struct event_move *move;
};

#define MOVER(work) ((struct mover_vars *)&(work)->vars)

/* each slot's mover, or null for a free slot. a mover frees its slot as it
 * ends. event_move_stop and event_move_stop_all free the slots of the
 * movers they destroy. a mover destroyed any other way leaves its slot set */
extern struct task *event_movers[EVENT_MOVERS];

/* move target in a line from from_x, from_y and from_z to to_x, to_y and
 * to_z. each step moves it an equal part, and the first leaves it a part
 * past from. target holds the angles it has at the call */
void event_move(struct task *target, Float from_x, Float from_y, Float from_z,
                Float to_x, Float to_y, Float to_z, int steps);
/* move target as event_move does, and turn it by an equal turn each step.
 * the turn runs from ang_x, ang_y and ang_z toward to_ang_x, to_ang_y and
 * to_ang_z. event_move_turn_degrees takes the angles in degrees, and
 * event_move_turn as angles. each step's turn is whole angle units, so
 * target can end off the angles it turns toward */
void event_move_turn_degrees(struct task *target, Float from_x, Float from_y,
                             Float from_z, Float to_x, Float to_y, Float to_z,
                             Float ang_x, Float ang_y, Float ang_z,
                             Float to_ang_x, Float to_ang_y, Float to_ang_z,
                             int steps);
void event_move_turn(struct task *target, Float from_x, Float from_y,
                     Float from_z, Float to_x, Float to_y, Float to_z,
                     Angle ang_x, Angle ang_y, Angle ang_z, Angle to_ang_x,
                     Angle to_ang_y, Angle to_ang_z, int steps);
/* keep target at x, y and z from base's place, an offset that doesn't turn
 * with base. target holds the angles it has at the call */
void event_follow(struct task *target, Float x, Float y, Float z, int steps,
                  struct task *base);
/* follow base as event_follow does, and turn target toward ang_x, ang_y and
 * ang_z. the turn starts from target's angles at the call. each step turns
 * it an equal turn of whole angle units */
void event_follow_turn(struct task *target, Float x, Float y, Float z,
                       Angle ang_x, Angle ang_y, Angle ang_z, int steps,
                       struct task *base);
/* follow base as event_follow does, with target at base's angles plus ang_x,
 * ang_y and ang_z */
void event_follow_angles(struct task *target, Float x, Float y, Float z,
                         Angle ang_x, Angle ang_y, Angle ang_z, int steps,
                         struct task *base);
/* follow base as event_follow does. target's angles about x, y and z are
 * atan2f(y, z), atan2f(z, x) and atan2f(y, x) as angles. each step adds how
 * far base has turned since the call */
void event_follow_facing(struct task *target, Float x, Float y, Float z,
                         int steps, struct task *base);
/* keep target at player's first or second weld point plus x, y and z. the
 * sum takes the point's x negated, and turns as the player's model does.
 * target sits that far from the player's place raised by its center_y.
 * target's angles stay as they are */
void event_follow_weld0(struct task *target, int player, Float x, Float y,
                        Float z, int steps);
void event_follow_weld1(struct task *target, int player, Float x, Float y,
                        Float z, int steps);
/* end target's moves now: destroy each mover that sets target, and free its
 * slot */
void event_move_stop(struct task *target);
/* end every move now, as event_move_stop does */
void event_move_stop_all(void);

#endif

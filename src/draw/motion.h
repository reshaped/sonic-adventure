/* motions: a model playing a table of actions, blending each into the next */
#ifndef DRAW_MOTION_H
#define DRAW_MOTION_H
#include <ninja.h>
#include <sg_xpt.h>

struct chaos;
struct chaos_form;
struct weld;
struct task_work;

/* a motion's state, as motion_step reads it. motion_create sets
 * MOTION_START, motion_step sets MOTION_CHANGE and MOTION_BLENDING, and the
 * owner sets MOTION_FREE. every other value is a mode an entry gives its
 * action, which says how the frame moves each step. below, the next entry
 * is actions[next], next being the playing entry's */
enum motion_mode {
    /* allocate the blend, then change to the entry asked for */
    MOTION_START = 0,
    /* start a blend into the entry asked for, or cut to it */
    MOTION_CHANGE = 1,
    MOTION_BLENDING = 2,
    MOTION_LOOP = 3, /* forward, wrapping round from the end to the start */
    MOTION_ONCE = 4, /* forward to the last frame, then the next entry */
    MOTION_BACKWARD = 6, /* backward to frame 0, then the next entry */
    MOTION_PACED = 9, /* either way by speed, wrapping round at both ends */
    MOTION_AT = 10, /* at pace's share of the way through the action */
    MOTION_STEP = 11, /* forward by pace each step, then the next entry */
    MOTION_POSED = 12, /* left alone: the owner sets the frame */
    MOTION_FREE = 14, /* free the blend: the motion's last step */
    MOTION_HOLD = 15, /* forward to the last frame, which it holds */
    MOTION_STILL = 16 /* left alone */
};

/* motion_flags' bits. motion_step sets MOTION_ENDED as a MOTION_LOOP action
 * wraps round, and as a MOTION_ONCE or MOTION_BACKWARD action ends. it flips
 * MOTION_HOLDING each step a MOTION_HOLD action holds its last frame */
#define MOTION_ENDED 0x1
#define MOTION_HOLDING 0x2

/* an entry of the table a motion plays */
struct motion_entry {
    NJS_ACTION *action;
    Uint8 nodes; /* in the action's object tree */
    Sint8 mode; /* an enum motion_mode */
    /* the entry played once this one ends, in MOTION_ONCE, MOTION_BACKWARD
     * or MOTION_STEP */
    Uint8 next;
    Float blend; /* the share of the blend into it a step adds: 1 cuts to it */
    Float speed; /* the frames of its action it plays each step */
};

/* how a model plays its table of actions, and the welds that close its
 * seams. an owner's record begins with its struct motion: motion_create
 * allocates the whole record */
struct motion {
    /* MOTION_AT's share of the action, or MOTION_STEP's frames a step */
    Float pace;
    Sint16 state; /* an enum motion_mode */
    Sint16 weld_mode; /* PLAYER_WELD_MAKE, PLAYER_WELDING or PLAYER_WELD_FREE */
    Uint16 wanted; /* the index in actions of the entry asked for */
    Uint16 current; /* the entry playing. a change sets it to wanted */
    Uint16 previous; /* current as it was before the last motion_step */
    /* the steps since one began a change. motion_step sets it to 0 where it
     * finds current unlike wanted outside a blend, and adds 1 at any other
     * step but MOTION_FREE's */
    Sint16 steps;
    /* the action's frame, or while state is MOTION_BLENDING, the blend's
     * share, from 0 to 1 */
    Float frame;
    Uint8 unk_14[0xc];
    Float speed; /* MOTION_PACED's rate, which scales the entry's speed */
    Uint8 unk_24[8];
    struct motion_entry *actions;
    struct weld *welds; /* a table a weld with a null base ends, or null */
    /* the two-frame action motion_step blends through, which MOTION_START
     * allocates and MOTION_FREE frees */
    NJS_ACTION *blend;
};

/* a point the boss tracks on its model, as chaos_track_points finds it
 * each step */
struct boss_point {
    NJS_POINT3 pos;
    NJS_POINT3 last; /* pos the step before */
    NJS_VECTOR axis; /* an axis of its node */
};

#define BOSS_POINTS 32

/* motion.c defines the bosses' shared state as well as motion_flags, and
 * this header declares it all. SHC lays out bss by each name's first
 * declaration, so the order here is the image's.
 *
 * motion_flags holds MOTION_ENDED and MOTION_HOLDING. the bosses read and
 * clear them */
extern Sint8 motion_flags;
/* the work of the boss that is fighting. the bosses, the code they share
 * and the camera read it */
extern struct task_work *boss_work;
/* a boss's mode changes through these. code that asks for a mode sets
 * boss_next_mode, and the boss takes it once it differs from the work's
 * mode. boss_last_mode keeps the mode the boss leaves. many modes go back
 * to boss_return_mode when they end. chaos_setup starts boss_next_mode at
 * the work's mode */
extern Sint8 boss_last_mode;
extern Sint8 boss_next_mode;
extern Sint8 boss_return_mode;
/* the calls of chaos0_attack_due since the count last went back to 0. it
 * goes back to 0 as Chaos 0 begins to walk. chaos0_attack_due puts it back
 * to 0 when it finds the count past 180, CHAOS0_CAN_ATTACK set and player 0
 * within range */
extern Sint32 boss_attack_checks;
/* the times a Chaos has left to repeat the move it is making */
extern Sint8 boss_repeats;
/* set as a Chaos boss sets up. cleared once the boss has shown its name and
 * the fight begins */
extern Sint8 boss_waiting;
/* set as the event before a Chaos fight hands Chaos to the boss. cleared as
 * the boss sets up */
extern Sint8 chaos_handed_over;
extern Uint8 dat_8c6c042b;
/* these four are the fighting Chaos's. chaos_current_form is its form and
 * chaos_record its record. chaos_params is the form's row of chaos_forms,
 * and chaos_travel how far chaos_steer has moved it */
extern Sint8 chaos_current_form;
extern struct chaos *chaos_record;
extern struct chaos_form *chaos_params;
extern Float chaos_travel;
extern struct boss_point boss_points[BOSS_POINTS];

/* allocate a zeroed record of size bytes, which must be at least
 * sizeof (struct motion). its state starts at MOTION_START and its
 * weld_mode at PLAYER_WELD_MAKE. returns the record, for the owner to free,
 * or null if it can't be allocated. either way it sets 0x1 in work->flags
 * and clears motion_flags. the owner sets actions and welds before the
 * first step, and keeps both alive. some of the game's callers leave the
 * result unchecked */
struct motion *motion_create(struct task_work *work, int size);
/* advance the motion one step. the first step allocates the blend, with
 * room for 118 nodes. to free the blend, the owner sets state to
 * MOTION_FREE and steps once more, before it frees the record. entry
 * indices, node counts and the blend's allocations go unchecked. the
 * actions' data must stay loaded while the motion plays them */
void motion_step(struct task_work *work, struct motion *motion);
/* act on the welds by weld_mode. PLAYER_WELD_MAKE copies each weld's
 * vertices into a buffer it allocates, finds its nodes, and moves weld_mode
 * on to PLAYER_WELDING. PLAYER_WELDING poses the entry asked for, or the
 * blend while blending. it then moves the shared vertices of each weld on
 * that entry's object, in place. PLAYER_WELD_FREE, or any other weld_mode,
 * writes the copies back and frees the buffers. only that case needs welds
 * non-null. the welds, which motion_weld writes, and their models must last
 * until then. vertex indices and allocations go unchecked */
void motion_weld(struct task_work *work, struct motion *motion);

#endif

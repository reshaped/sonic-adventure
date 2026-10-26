/* event records and actors. a task's event record queues the motions it
 * plays and the paths it runs, and keeps its mouth's place in a text. for a
 * character, it also holds what to do and where to look. an actor is a task
 * made to play its record */
#ifndef EVENT_EVENT_H
#define EVENT_EVENT_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* an event record's flags */
#define EVENT_GROUND 0x1 /* stand on the ground */
#define EVENT_SLOPE 0x2 /* lean with the slope */
#define EVENT_IGNORE_GROUND 0 /* neither: keep its own height and tilt */

/* a queued motion's or path's flags */
#define EVENT_REPEAT 0x1 /* at the end, start over unless another is queued */
/* a path with this flag, queued while no path runs, counts its points from
 * where the task stands, not from zero. as it ends, the next path counts its
 * points from where the task stands then. without the flag, both count from
 * zero */
#define EVENT_FROM_HERE 0x2

/* where a character looks, in its record's look */
enum event_look {
    EVENT_LOOK_NONE,
    EVENT_LOOK_ANGLES, /* the way look_ang turns */
    EVENT_LOOK_POINT, /* at look_pos */
    EVENT_LOOK_TASK /* at look_task, offset by look_pos for a player */
};

/* what an event has a character do, in its record's action */
enum event_action {
    EVENT_ACTION_NONE,
    EVENT_ACTION_DONE, /* the player has done the action, and stops */
    EVENT_ACTION_RUN, /* run to move.pos */
    EVENT_ACTION_MOVE, /* go the way move.pos points, as fast as it's long */
    EVENT_ACTION_PUSH, /* be pushed by move.pos, an acceleration */
    EVENT_ACTION_TURN_BY, /* turn by move.ang.y */
    EVENT_ACTION_TURN_TO /* turn to move.ang.y */
};

/* a motion in an event record's queue */
struct event_motion {
    Uint8 flags; /* EVENT_REPEAT */
    Uint8 blend; /* draws spent blending in from event_work.previous, or 0 */
    NJS_ACTION action;
    NJS_TEXLIST *texlist; /* or null to keep the ones set */
    struct event_motion *next;
    Float speed; /* motion frames advanced each draw */
};

/* a path point, its angles in 256ths of a turn */
struct event_path_point {
    Sint8 ang_x;
    Sint8 ang_y;
    Sint8 ang_z;
    NJS_POINT3 pos;
};

/* a path of count points, which runs a point a step at a speed of 1 */
struct event_path {
    Sint32 count;
    struct event_path_point *points;
};

/* a path in an event record's queue */
struct event_path_entry {
    Uint8 flags; /* EVENT_REPEAT, EVENT_FROM_HERE */
    struct event_path *path;
    Float speed; /* points advanced each step */
    struct event_path_entry *next;
};

/* a key of a mouth script: over its steps, the mouth moves from its shape
 * to the key's. a key of 0 steps ends the script */
struct event_mouth_key {
    Sint8 steps;
    Sint8 shape;
};

/* the mouth scripts, numbered as event_mouth_start says. a number from
 * EVENT_MOUTH_SCRIPTS on reads past the table, the @bug in event.c */
#define EVENT_MOUTH_SCRIPTS 31
extern struct event_mouth_key *event_mouth_scripts[EVENT_MOUTH_SCRIPTS];

/* a mouth that moves with a text, a key of a mouth script at a time */
struct event_mouth {
    Sint8 shape; /* where the key moves from */
    Sint8 next_shape; /* the key's own, where it moves to */
    Sint16 steps; /* the key's steps, or 0 while the mouth is still */
    Sint16 timer; /* the steps into the key */
    const char *text; /* the bytes still to take, or null */
    struct event_mouth_key *script; /* the next key */
};

/* a task's event record, at work->event */
struct event_work {
    Uint8 flags; /* EVENT_GROUND, EVENT_SLOPE */
    Uint8 unk_01; /* fun_8c04d0c6 sets it, and nothing reads it */
    Uint8 unk_02[2]; /* nothing reads or writes them */
    Uint8 look; /* enum event_look */
    struct angles look_ang;
    NJS_POINT3 look_pos;
    struct task *look_task;
    /* enum event_action. a player sets it to EVENT_ACTION_DONE as it ends a
     * run, move or push, and to EVENT_ACTION_NONE once it has stopped or
     * turned */
    Uint8 action;
    union {
        NJS_POINT3 pos;
        struct angles ang;
    } move;
    Float speed; /* to run at, or 0 for the character's own */
    /* the most a run speeds up by in a step, or 0 for the character's own */
    Float accel;
    /* the first motion's frame, or in its blend the draws done */
    Float motion_frame;
    struct event_motion *motions; /* queued, the first one playing */
    /* the last motion to end, kept to blend from until a blend frees it */
    struct event_motion *previous;
    /* the first path's point index, with the part of the way to the next as
     * its fraction */
    Float path_frame;
    struct event_path_entry *paths; /* queued, the first one running */
    NJS_POINT3 origin; /* where the running path's points count from */
    struct event_mouth mouth;
    /* where an actor draws, from its position and turned with it */
    NJS_POINT3 offset;
    Float shadow; /* an actor's shadow size, or 0 for none */
};

/* give work a zeroed event record, with EVENT_GROUND and EVENT_SLOPE set,
 * unless it has one. allocation failure is unchecked */
void event_alloc(struct task_work *work);
/* free work's event record, if it has one, with its queues and
 * event_work.previous, and set work->event to null */
void event_free(struct task_work *work);

/* each call below that takes work needs its event record, except
 * event_motions_draw and event_paths_step, which do nothing without one */

/* free the queued motions and event_work.previous */
void event_motions_clear(struct task_work *work);
/* queue a motion behind the others. allocation failure is unchecked */
void event_motions_add_action(struct task_work *work, NJS_ACTION *action,
                              NJS_TEXLIST *texlist, Float speed, int flags,
                              int blend);
void event_motions_add(struct task_work *work, NJS_OBJECT *object,
                       NJS_MOTION *motion, NJS_TEXLIST *texlist, Float speed,
                       int flags, int blend);
/* draw the first queued motion in the current matrix, and advance it by its
 * speed. a motion that ends becomes event_work.previous, and the one there
 * leaks unless a blend freed it. a motion with a blend, while
 * event_work.previous is set, first spends blend draws blending into its
 * frame 0. the blend runs from the previous motion's last frame, or its
 * frame 0 if the previous motion repeats. the blend then frees the previous
 * motion. with a player's work, the player's welds follow the motion drawn,
 * or the previous one for the first half of a blend. the draw shades with
 * DRAW_PLAYER_PALETTE, and leaves palette 0 set */
void event_motions_draw(struct task_work *work);
/* queue path behind the others, borrowing it until it ends or is cleared.
 * allocation failure is unchecked */
void event_paths_add(struct task_work *work, struct event_path *path,
                     Float speed, int flags);
/* free the queued paths */
void event_paths_clear(struct task_work *work);
/* place the task part of the way between two points of the first path,
 * counted from event_work.origin, and advance the path by its speed. the
 * points' angles turn as njRotateXYZ turns by them. the task takes the
 * angles that make the same turn in an actor's draw, and a player aims by
 * the task's ang.y. EVENT_GROUND and EVENT_SLOPE then apply, by the ground
 * below. a path that ends is freed */
void event_paths_step(struct task_work *work);
/* move the mouth with the bytes of text. A to Z take scripts 0 to 25 of
 * event_mouth_scripts, and a to z 26 to 51. any other byte takes the script
 * its code numbers, but a '0', or a code from 0x80 on, stops the mouth. an
 * A, and the end of text, take script 0 three times in four and script 1
 * otherwise. text is borrowed until its end or the mouth stops. a still
 * mouth starts on its next step, and a moving one takes text once its
 * script ends */
void event_mouth_start(struct task_work *work, const char *text);
/* advance a moving mouth a step. at the end of a key, the mouth takes the
 * key's shape and the next key. at the end of a script, it takes the next
 * byte's */
void event_mouth_step(struct task_work *work);
/* stop the mouth, and let go of its text */
void event_mouth_stop(struct task_work *work);
/* create an actor: a task at level 5 with an event record and a collision
 * sphere, which event_actor_step steps. a null from task_create is
 * unchecked */
struct task *event_actor_create(void);
/* an actor's step: move the mouth, and run the path. with no motion queued,
 * that is all. otherwise EVENT_GROUND and EVENT_SLOPE apply, and the actor
 * draws its motion, and its shadow on the ground below. it draws turned by
 * ang.z, then ang.x, then a half turn less ang.y. it registers its collision
 * only while its radius is above 0. any task with an event record and a
 * collision can take this step */
void event_actor_step(struct task *task);

#endif

/* playing events: an event's module, the thread that plays it, and the
 * calls its script makes */
#ifndef EVENT_PLAY_H
#define EVENT_PLAY_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

struct event;
struct event_path;
struct texture_pvm;

/* the head of an event's module, at its load address */
struct event_module {
    void (*entry)(int phase);
    /* loaded for EVENT_LOAD and released after EVENT_END. the list ends at a
     * null texlist */
    struct texture_pvm *textures;
};

/* the phases an event's entry is called with. an entry may ignore any */
enum event_phase {
    EVENT_LOAD, /* once the module's textures are loaded */
    EVENT_PLAY, /* on the event's thread */
    EVENT_END, /* on the thread, after EVENT_PLAY returns or a skip */
    EVENT_UNLOAD, /* once the event's thread, if any, is destroyed */
    EVENT_NONE /* never passed: an event's status while it has no thread */
};

/* how a move of the event camera runs from where it was to where it goes */
enum event_ease {
    EVENT_EASE_LINEAR, /* at an even pace */
    EVENT_EASE_SMOOTH /* slow at both ends, along half a cosine */
};

/* what the event camera aims at */
enum event_camera_look {
    EVENT_CAMERA_LOOK_NONE,
    EVENT_CAMERA_LOOK_POINT, /* look_pos */
    /* look_pos from look_task, as event_camera_look_at_task places it */
    EVENT_CAMERA_LOOK_TASK
};

/* how the event camera's pos_to moves each step */
enum event_camera_follow {
    EVENT_CAMERA_FOLLOW_NONE, /* it stays */
    EVENT_CAMERA_FOLLOW, /* by as much as follow_task moves */
    /* with follow_task, turning about it as it turns about y */
    EVENT_CAMERA_FOLLOW_HEADING,
    /* with follow_task, turning about it as it turns every way */
    EVENT_CAMERA_FOLLOW_LOCAL,
    EVENT_CAMERA_FOLLOW_ORBIT, /* around follow_task, by event_camera_orbit */
    EVENT_CAMERA_FOLLOW_PATH /* along path */
};

/* a key of a camera path: the camera's angles and position there */
struct event_camera_key {
    Sint16 ang_y;
    Sint16 ang_x;
    Sint16 ang_z;
    NJS_POINT3 pos;
};

/* a camera path of count keys, which runs a key a step at a speed of 1 */
struct event_camera_path {
    Uint8 unk_00[4];
    Float count;
    struct event_camera_key *keys;
};

/* the camera while an event has it. pos moves from pos_from to pos_to,
 * eased by pos_ease, as pos_step counts up to pos_steps. it then stays on
 * pos_to. ang and fov move the same way, by their own fields */
struct event_camera {
    Uint8 pos_ease; /* enum event_ease, which an orbit eases by too */
    Uint8 ang_ease; /* enum event_ease */
    Uint16 pos_steps;
    Uint16 pos_step;
    Uint16 ang_steps;
    Uint16 ang_step;
    NJS_POINT3 pos;
    struct angles ang;
    NJS_POINT3 pos_from;
    struct angles ang_from;
    NJS_POINT3 pos_to; /* which the follow mode moves each step */
    struct angles ang_to; /* which the look and a path move each step */
    Uint8 look; /* enum event_camera_look */
    NJS_POINT3 look_pos; /* a point, or for LOOK_TASK an offset */
    struct task *look_task; /* borrowed while look is LOOK_TASK */
    Uint8 follow; /* enum event_camera_follow */
    Uint16 orbit_steps;
    Uint16 orbit_step;
    /* follow_task's position and angles at the last step. an orbit keeps
     * its height in follow_pos.y and starts from follow_ang */
    NJS_POINT3 follow_pos;
    struct angles follow_ang;
    Float orbit_start_distance;
    Uint8 unk_8c[12];
    struct angles orbit_end_ang;
    Float orbit_end_distance;
    /* borrowed while follow is a mode with a task */
    struct task *follow_task;
    Uint8 fov_ease; /* enum event_ease */
    Uint16 fov_steps;
    Uint16 fov_step;
    Angle fov;
    Angle fov_from;
    Angle fov_to;
    struct event_camera_path *path; /* borrowed with its keys while PATH runs */
    /* the key the path is at, with the part of the way to the next as its
     * fraction */
    Float path_frame;
    Float path_speed; /* keys a step */
    struct event *owner; /* the event that took it */
};

/* a step of a pad recording */
struct event_pad_key {
    Uint8 buttons;
    Sint8 ang; /* the stick's direction, in 256ths of a turn */
    Uint8 stroke; /* how far the stick is pushed, out of 255 */
};

struct event_pad_recording {
    const struct event_pad_key *keys;
    Sint32 count;
};

extern struct event_camera event_camera;

/* forget every event played and the loaded module, without unloading it.
 * the messages show, and no event has control */
void event_init(void);
/* while an event has control, a press of Y on the first pad connected
 * hides or shows the messages */
void event_toggle_messages(void);
/* pad_enable and pad_disable, for an event's script */
void event_pads_on(void);
void event_pads_off(void);
/* event_take_control gives control to the calling thread's event, unless an
 * event has it already. event_release_control takes control back only from
 * that event */
void event_take_control(void);
void event_release_control(void);
/* whether a field may start its music: TRUE while no event plays, and
 * otherwise FALSE. the walk of music_events stops only at a match, the @bug
 * in play.c. for an event the list doesn't hold, it reads past the list
 * until a word equals the event's number */
Bool event_check_music(void);
/* load event number's module and its textures, and call its entry with
 * EVENT_LOAD. a number below 0 takes the module already in memory. the
 * streams stop for a read, and start over after the load. a read that fails
 * calls no entry and leaves the module's memory zeroed */
void event_load(int number);
/* destroy the loaded event's thread, if any, call its entry with
 * EVENT_UNLOAD and forget the module. the textures stay loaded, unless the
 * event's end released them */
void event_unload(void);
/* play the loaded event and mark it played, cutting short any message. if
 * the event has a thread already, put the game back as an event's end does
 * instead. either way, the pad recorder stops */
void event_start(void);
/* if the event in control is in EVENT_PLAY, end it: its thread starts over
 * at EVENT_END */
void event_skip(void);
/* TRUE while the loaded event has a thread, and otherwise FALSE. the
 * argument is ignored */
Bool event_is_playing(int unused);
/* TRUE while an event has taken control, and otherwise FALSE. while one
 * has, the stage hides the HUD, doesn't count the time and doesn't pause */
Bool event_in_control(void);
/* TRUE once event number has played, and otherwise FALSE */
Bool event_was_played(int number);

/* the calls an event's script makes. a call that uses the calling thread's
 * data must run on the event's thread. those are the calls that wait, the
 * event_message calls, and the calls that take or release control or the
 * camera. a call that takes a task ignores a null one, but an event_camera
 * call needs one. a call that reaches the task's event record needs the
 * task to have one */

/* the count of sound ids from the address of the event thread's data on.
 * event_sound_play takes them in turn, and a mask wraps the turn, so the
 * count must be a power of two. the event's end stops every sound under
 * them */
#define EVENT_SOUND_PLAY_IDS 32

/* wait steps steps, drawing the thread's window each step. steps must not
 * be negative */
void event_wait(int steps);
/* show text in the thread's window, waiting out the fades. a window that
 * isn't up fades in with the text. one that is up fades its old text out,
 * if any shows, then fades text in */
void event_message_show(const char *text);
/* show text, then wait out what's left of steps */
void event_message_wait(int steps, const char *text);
/* event_message_show and event_message_wait for formatted text. the text
 * must fit in 127 bytes, since the formatter checks no bound */
void event_message_printf(const char *format, ...);
void event_message_wait_printf(int steps, const char *format, ...);
/* event_message_close fades the window out and releases it.
 * event_message_clear fades only its text out */
void event_message_close(void);
void event_message_clear(void);
/* offer the lines of text and wait for a choice, drawing the thread's
 * window meanwhile. return the line chosen, counting from 1 */
int event_message_choose(const char *text);

Sint32 event_load_texlist(NJS_TEXLIST *texlist);
Sint32 event_release_texlist(NJS_TEXLIST *texlist);
/* the task in player slot player, or null. a slot, not a character */
struct task *event_get_player(int player);
/* put a new task in *slot: an actor, or what create makes. a task the slot
 * held is scheduled for destruction. the new task takes the position and
 * angles given, ang_y as its own ang.y, not as event_set_ang takes it.
 * creation must succeed and give the task work, unchecked */
void event_place_actor(struct task **slot, Float x, Float y, Float z,
                       Angle ang_x, Angle ang_y, Angle ang_z);
void event_place_task(struct task **slot, struct task *(*create)(void), Float x,
                      Float y, Float z, Angle ang_x, Angle ang_y, Angle ang_z);
/* schedule *slot, if any, for destruction, and clear the slot now. the
 * task lives until its next step */
void event_kill_task(struct task **slot);
/* stop everything the event has the task do: its motions, paths, look, run
 * and talk */
void event_clear_task(struct task *task);
/* move the task to x, y, z. a player's latest history entry moves there
 * too */
void event_set_pos(struct task *task, Float x, Float y, Float z);
/* the task's ang.y becomes a quarter turn less ang_y, here and in
 * event_create_player */
void event_set_ang(struct task *task, Angle ang_x, Angle ang_y, Angle ang_z);
/* set the task's event flags: EVENT_GROUND, EVENT_SLOPE, both or
 * EVENT_IGNORE_GROUND */
void event_set_flags(struct task *task, int flags);
/* an actor's offset: where it draws, and its collision sphere's center,
 * from its position */
void event_set_offset(struct task *task, Float x, Float y, Float z);
/* an actor's shadow size, or 0 for none */
void event_set_shadow(struct task *task, Float size);
void event_set_radius(struct task *task, Float radius);
/* make a player's task in slot player, at the position and angles given,
 * unless the slot has one. creation is unchecked */
void event_create_player(int player, void (*step)(struct task *task), Float x,
                         Float y, Float z, Angle ang_x, Angle ang_y,
                         Angle ang_z);
void event_kill_player(int player);
/* stop everything the event has the player do, then put it at rest in
 * PLAYER_MODE_RESET. a player holding a task lets go of it. the player in
 * slot 0, whatever its character, also loses work flag 0x200, Sonic's
 * charged light dash */
void event_reset_player(int player);
/* queue a motion. the action is copied, but texlist and the action's
 * object and motion must stay loaded until the motion ends. the motion data
 * must also stay until the next motion has blended from it. allocation
 * failure is unchecked */
void event_play_action(struct task *task, NJS_ACTION *action,
                       NJS_TEXLIST *texlist, Float speed, int flags, int blend);
void event_play_motion(struct task *task, NJS_OBJECT *object,
                       NJS_MOTION *motion, NJS_TEXLIST *texlist, Float speed,
                       int flags, int blend);
void event_clear_motions(struct task *task);
/* wait until the task's motions have all ended, which a repeating last
 * motion never does. task and its work must live through the wait */
void event_wait_motions(struct task *task);
/* path and its points are borrowed until the path ends or is cleared */
void event_play_path(struct task *task, struct event_path *path, Float speed,
                     int flags);
void event_clear_paths(struct task *task);
void event_wait_paths(struct task *task);
/* set event_work.unk_01, which nothing reads */
void fun_8c04d0c6(struct task *task, int value);
/* play recording into port, a key a step, on a child of the event's
 * thread, and return two steps later. recording and its keys are borrowed
 * until it ends, and its count must not be negative. every key holds Start
 * too, and press and release go by pad_records[0].old, whatever the port */
void event_play_pad(int port, const struct event_pad_recording *recording);
/* wait until Start is up on port, both now and at the read before */
void event_wait_start_released(int port);
/* set the look or the action in the task's event record. event_run_wait
 * waits until the action is EVENT_ACTION_NONE */
void event_look_clear(struct task *task);
void event_look_at_task(struct task *task, struct task *target, Float x,
                        Float y, Float z);
void event_look_at(struct task *task, Float x, Float y, Float z);
void event_look_angles(struct task *task, Angle ang_x, Angle ang_y,
                       Angle ang_z);
void event_run_clear(struct task *task);
void event_run_to(struct task *task, Float x, Float y, Float z);
void event_run_to_speed(struct task *task, Float x, Float y, Float z,
                        Float speed, Float accel);
void event_turn_by(struct task *task, Angle ang_x, Angle ang_y, Angle ang_z);
void event_turn_to(struct task *task, Angle ang_y);
void event_run_wait(struct task *task);

/* event_camera_take gives the camera to the calling thread's event, even
 * from another event. the camera starts where the game's is, with no move,
 * look or follow. event_camera_release takes the camera back only from that
 * event */
void event_camera_take(void);
void event_camera_release(void);
/* move the event camera a step along its follow, look and eases, then set
 * camera_work's position and angles to its own. camera_step_event runs it */
void event_camera_step(void);
/* each call below that takes steps begins a move over steps steps, eased
 * by ease. a move starts from where the camera is, but an orbit's. 0 steps
 * moves the position or the angles at once */
void event_camera_set_ang(enum event_ease ease, int steps, Angle ang_x,
                          Angle ang_y, Angle ang_z);
void event_camera_set_pos(enum event_ease ease, int steps, Float x, Float y,
                          Float z);
/* move to the offset from the task's position, in world axes, as the task
 * stands now. task isn't kept */
void event_camera_set_pos_offset(enum event_ease ease, int steps,
                                 struct task *task, Float x, Float y, Float z);
/* the same, with the offset turned as the task stands now. it turns by
 * the task's z and x angles, then about y by a quarter turn less its ang.y */
void event_camera_set_pos_local(enum event_ease ease, int steps,
                                struct task *task, Float x, Float y, Float z);
/* the same, with the offset turned only about y, by a quarter turn less
 * the task's ang.y */
void event_camera_set_pos_heading(enum event_ease ease, int steps,
                                  struct task *task, Float x, Float y, Float z);
void event_camera_look_clear(void);
/* aim at the point x, y, z until the look changes. roll is the camera's z
 * angle */
void event_camera_look_at(enum event_ease ease, int steps, Float x, Float y,
                          Float z, Angle roll);
/* the same, at an offset from the task's position. the offset turns by
 * the task's z and x angles, then about y by its ang.y and a quarter turn.
 * task and its work are borrowed until the look changes */
void event_camera_look_at_task(enum event_ease ease, int steps,
                               struct task *task, Float x, Float y, Float z,
                               Angle roll);
void event_camera_follow_clear(void);
/* these borrow task and its work until the follow mode changes */
void event_camera_follow(struct task *task);
void event_camera_follow_local(struct task *task);
void event_camera_follow_heading(struct task *task);
/* circle task. the camera stands height above it and distance from it,
 * along z turned by the angles in the order z, x and y. it goes there at
 * once. over steps steps, the angles then turn to the second ones and the
 * distance goes to to_distance. task and its work are borrowed until the
 * follow mode changes */
void event_camera_orbit(enum event_ease ease, int steps, struct task *task,
                        Float height, Angle ang_x, Angle ang_y, Angle ang_z,
                        Angle to_x, Angle to_y, Angle to_z, Float distance,
                        Float to_distance);
/* ease the field of view to fov. 0 steps changes nothing, since the
 * perspective changes only on a step of the ease. ease is an event_ease,
 * though the parameter is a byte */
void event_camera_set_fov(Sint8 ease, int steps, Angle fov);
/* move along path, speed keys a step, until the path ends or the follow
 * mode changes. path and its keys are borrowed until then. path->count must
 * be a whole number, at least 1. only a positive speed ends the path, and a
 * negative one reads before the keys. a look, while set, takes over the
 * path's y and x angles */
void event_camera_set_path(struct event_camera_path *path, Float speed);
void event_camera_wait_path(void);

void event_talk_start(struct task *task, const char *text);
void event_talk_stop(struct task *task);

/* the voice that plays as each of the 13 upgrades is taken: a chime, not
 * speech */
#define VOICE_UPGRADE_CHIME 580

/* event_voice_wait waits until the voice ends. event_voice_play plays voice
 * once the one before it ends */
void event_voice_wait(void);
void event_voice_play(int voice);
void event_voice_stop(void);

/* kill the task in slot, if any, and clear slot */
/* @hack spelling: the test and the kill as one expression, which SHC compiles
 * otherwise than an if statement; as an if, 36 of ev0001_script's 1790
 * words differ, and the count over the macro's other users is unmeasured */
#define EVENT_KILL(slot) ((slot) != NULL && (task_kill(slot), (slot) = NULL))

#endif

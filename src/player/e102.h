/* E-102: his modes, his record and his player task */
#ifndef PLAYER_E102_H
#define PLAYER_E102_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/pad.h"

struct task;
struct task_work;

/* line segments, drawn as one NJS_POINT3COL. indices names each segment's
 * two points in points, and colors holds each point's color */
struct lockon_strip {
    NJS_POINT3 *points;
    NJS_COLOR *colors; /* one per point */
    Sint16 *indices; /* two to a segment */
    Sint16 count; /* segments */
    NJS_POINT3COL lines; /* built from the tables, on the heap */
};

/* E-102's modes, in work->mode. e102_mode_names, in e102.c, names the
 * first 47 in this order */
enum e102_mode {
    E102_INIT,
    E102_STAND,
    E102_WALK,
    /* on a path, from a PLAYER_MODE_PATH or PLAYER_MODE_PATH_SPIN request */
    E102_PATH,
    E102_JUMP,
    E102_SPRING,
    E102_LAUNCH,
    E102_FALL,
    E102_STOP,
    E102_PUSH,
    E102_HURT,
    E102_BACK,
    E102_CRASH,
    E102_SLIDE,
    E102_POSE, /* still, in the action a request or his clear set */
    E102_LIFT,
    E102_PUT_DOWN,
    E102_HOLD_STAND,
    E102_GRIP, /* gripping a task, to push, pull or shake it */
    E102_HOLD_WALK,
    E102_THROW,
    E102_HOLD_STOP,
    E102_HOLD_FALL,
    E102_HOLD_JUMP,
    E102_HOLD_SPRING,
    E102_HOLD_LAUNCH,
    E102_PULL,
    E102_AIR_THROW,
    E102_SHAKE,
    /* E102_PRE_JUMP holds him on the ground till his jump's action, or his
     * carrying jump's, plays, and then he jumps */
    E102_PRE_JUMP,
    E102_HOLD_PRE_JUMP,
    E102_SKATE,
    E102_HOLD_SKATE,
    E102_WATER_SKATE,
    E102_HOLD_WATER_SKATE,
    E102_WATER_JUMP,
    E102_HOLD_WATER_JUMP,
    E102_SKATE_JUMP,
    E102_HOLD_SKATE_JUMP,
    E102_SKATE_STOP,
    E102_HOLD_SKATE_STOP,
    /* e102_leap's leap, the fall after it, and the hover he stops in once
     * he falls near the ground */
    E102_LEAP_START,
    E102_LEAP,
    E102_LEAP_FALL,
    E102_LEAP_HOVER,
    /* a step each: the start of a skate, and a brake out of one, as
     * e102_skate_start and e102_skate_brake ask */
    E102_SKATE_START,
    E102_SKATE_BRAKE,
    E102_LEDGE, /* hanging from a ledge, still */
    E102_LEDGE_GO, /* hanging from a ledge, moving along it */
    E102_TIME_UP, /* his time is up, and he goes down */
    E102_UNK_50,
    E102_DOWN,
    /* his stage clear's pose. it ends in the flash, blast and smoke of a
     * shot, with no shot, and then E102_POSE */
    E102_CLEAR,
    /* still in a module's action, till PLAYER_MODE_RIDE_OFF */
    E102_MODULE_POSE,
    /* shaking a CARRY_KIND_SMALL task he holds. the grab button with the
     * stick starts it, and letting the stick go ends it */
    E102_HOLD_SHAKE,
    E102_EVENT, /* an event runs him along its paths */
    E102_EDIT /* the set editor has him */
};

/* struct e102's aim_mode. the action button starts an aim in a mode he can
 * aim in, and he locks on while it's held. once it's let go, e102_shoot
 * fires at his lock-ons when the aim is ready again. running out of aim
 * time or game time, or a mode he can't aim in, cancels the aim and
 * releases every lock-on */
enum e102_aim_mode {
    E102_AIM_INIT,
    E102_AIM_READY,
    E102_AIM_START,
    E102_AIM_LOCKING,
    E102_AIM_CANCEL,
    E102_AIM_LET_GO,
    E102_AIM_CANCELLED
};

/* struct e102's shot_phase, which e102_shoot steps through */
enum e102_shot_phase {
    E102_SHOT_START,
    E102_SHOT_TURN, /* facing the nearest lock-on till the aim is ready */
    E102_SHOT_MARK, /* marking the lock-ons to fire at */
    E102_SHOT_NEXT, /* facing the nearest marked lock-on */
    E102_SHOT_AIM, /* his shot's action, up to the frame the shot leaves at */
    E102_SHOT_KICK /* the shot's kick, then E102_SHOT_NEXT */
};

/* how many steps of his stick struct e102 keeps */
#define E102_STICK_STEPS 8

/* in struct e102's flags: the skate's sound is playing */
#define E102_SKATE_SOUND 0x1

/* E-102's own record, at record->own.e102.record[0] from his init until
 * his destructor frees it */
struct e102 {
    Sint8 aim_mode; /* enum e102_aim_mode */
    Sint8 shot_phase; /* enum e102_shot_phase */
    Sint8 unshot; /* lock-ons still to shoot, as nearest_lockon last counted */
    Uint8 unk_03;
    Sint8 lockon_count; /* the lock-ons in lockons */
    Sint8 shot_count; /* the shots in shots */
    Sint8 shot_way; /* enum shot_way, in e102_shot.c */
    Uint8 unk_07;
    Sint16 steps; /* the steps since his init or his last leap */
    /* the number his newest lock-on of this aim took. each target that
     * counts toward his bonus raises it by one, to 99. it's 0 till the
     * first, and letting go, a cancel or a hit sets it to 0 again */
    Sint16 lockon_number;
    Sint16 unk_0c; /* set to 0 with lockon_number, and read nowhere */
    Uint8 unk_0e[2];
    Sint16 aim_time; /* steps left to hold his aim */
    /* the draws left that turn his cannon to target, his newest lock-on. no
     * new lock-on comes till they run out. e102_draw counts one off each
     * time it runs, in his step and in each frame the game is paused, but
     * not while he blinks out */
    Sint16 aim_draws;
    Uint8 unk_14[2];
    Sint16 flags; /* E102_SKATE_SOUND */
    Float booster_lift; /* his booster's height in its bob */
    Angle booster_phase; /* the phase of the booster's bob */
    Angle booster_ang_x;
    Uint8 unk_24[4];
    Angle booster_ang_z;
    Angle bob; /* the phase of his body's bob as he skates on water */
    struct task *target; /* the lock-on the cannon turns to */
    NJS_POINT3 muzzle; /* where the cannon points from */
    NJS_VECTOR aim; /* the way the cannon points */
    Float beam_length;
    Float beam_reach; /* beam_length squared: how far the beam locks on */
    NJS_POINT3 shot_pos; /* where a shot leaves */
    NJS_VECTOR shot_dir; /* the way a shot leaves */
    struct task *search; /* the task that locks on as he aims */
    Float shot_frame; /* the frame of shot_action */
    Uint8 unk_74[12];
    NJS_ACTION *shot_action; /* drawn in place of his arm */
    Float cannon_frame; /* the frame of the cannon's action */
    Angle glow; /* the phase of his glowing material */
    Angle cannon_ang; /* the cannon's turn, about y */
    /* half of how far a shot's kick moves his body, arm and cannon. only
     * E102_SHOT_KICK sets it above 0. it falls on a line from 1 at the frame
     * the shot leaves at to 0 at the frame a whole kick ends */
    Float kick;
    /* 6 to either side of his model's node 2, along its z: where the smoke
     * puffs out as he fires */
    NJS_POINT3 points[2];
    Float fall_speed; /* his speed.y on his last step in E102_FALL */
    struct lockon_strip inner;
    struct lockon_strip outer;
    Uint8 unk_f0[4];
    /* his lock-ons, and his shots that home in on one. each adds itself to
     * its list in its own step, and e102_shoot empties both lists in his */
    struct task *lockons[32];
    struct task *shots[32];
    /* his stick on each of his last steps, the oldest first. while he locks
     * on, his step reads the oldest, so he follows the stick 7 steps late */
    struct pad_stick sticks[E102_STICK_STEPS];
    Float ground; /* the height of the ground below him */
    Float above; /* how far above the ground he is */
    Uint32 ground_flags; /* the ground's world flags */
    Float walked; /* how far he's walked since his last step's sound */
};

/* e102_hovering is e102.c's alone. bss follows each name's first
 * declaration, so declaring it here puts it after e102_in_play, as in the
 * image */
extern Sint8 e102_in_play; /* TRUE while no event holds the game */
extern Sint8 e102_hovering; /* TRUE while he hovers */
/* TRUE while his time counts down. a battle stops it. a volley bonus stops it
 * till the bonus ends, and the time bonus till its tally has added its
 * seconds */
extern Sint8 e102_time_runs;
extern NJS_POINT3COL e102_beam;
extern NJS_POINT3 e102_beam_points[];
extern NJS_COLOR e102_beam_colors[];
/* the record of the E-102 that stepped last */
extern struct e102 *e102_current;

/* E-102's player task step */
void e102_step(struct task *task);
/* TRUE in the modes E-102 can aim his cannon in */
Bool e102_can_aim(struct task_work *work);
/* a look task's step: the player's head and eyes turn to what it looks at,
 * and take their own angles back as the task ends. no task runs it */
void e102_look_step(struct task *task);

#endif

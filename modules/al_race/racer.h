/* a Chao in the race, one to a lane: what the race keeps of it, and the
 * commands it gives it */
#ifndef AL_RACE_RACER_H
#define AL_RACE_RACER_H
#include <ninja.h>
#include <sg_xpt.h>

#include "al_race/spray.h"

struct alife_chao;
struct racer;
struct race_path;
struct task;
struct task_work;

#define RACE_LANES 8

/* what a racer tells its Chao to do, which the Chao's mode takes with
 * racer_take_command. each name is its label on the race's debug page: RAPPA
 * is the bugle and JYOURO the watering can */
enum racer_command {
    COMMAND_NONE,
    COMMAND_WINDOW,
    COMMAND_BIRTH,
    COMMAND_WAIT,
    COMMAND_RACE,
    COMMAND_WALK,
    COMMAND_REST,
    COMMAND_HAPPY,
    COMMAND_SWING,
    COMMAND_PUSH,
    COMMAND_EAT,
    COMMAND_PLAY_RAPPA,
    COMMAND_PLAY_JYOURO,
    COMMAND_PLAY_SCOOP,
    COMMAND_RETURN,
    COMMAND_DANCE,
    COMMAND_COMMENDATION,
    COMMAND_DEBUG
};

/* where a racer stands in the race. racer_entry_step runs the first two
 * states, and racer_step the rest. a state with a command beside it issues
 * that command, and a RACER_TO_ state moves to the next as it runs */
enum racer_state {
    RACER_TO_ENTRY, /* COMMAND_WINDOW */
    RACER_IN_ENTRY,
    RACER_TO_WAIT, /* COMMAND_WAIT */
    RACER_WAITING,
    RACER_TO_REST, /* COMMAND_REST */
    RACER_RESTING,
    RACER_TO_WALK, /* COMMAND_WALK */
    RACER_WALKING, /* COMMAND_WALK, each step */
    RACER_TO_RACE, /* COMMAND_RACE */
    RACER_RACING,
    RACER_TO_GOAL, /* COMMAND_HAPPY */
    RACER_AT_GOAL,
    RACER_TO_PODIUM, /* COMMAND_COMMENDATION */
    RACER_ON_PODIUM, /* COMMAND_COMMENDATION, each step */
    RACER_TO_RESTART,
    RACER_RESTARTING,
    RACER_FINISHING /* past the course's distance, bound for the target */
};

/* bits of a racer's flags */
#define RACER_INITIALIZED 0x1 /* racer_init has run */
#define RACER_ATTACHED 0x2 /* racer_attach has run; racer_init clears it */
#define RACER_HAS_CHAO 0x4 /* both have run: it has its Chao */
/* its Chao was aimed at a target. a path update with it set takes only the
 * distance from the paths, and clears it. in RACER_FINISHING, while the path
 * has a target, a path update reads neither this flag nor RACER_HOLD_PATH */
#define RACER_AIMED 0x8
/* a path update with it set and RACER_AIMED clear leaves the target and the
 * distance as they are, and clears it */
#define RACER_HOLD_PATH 0x10
#define RACER_SPRAYED 0x20 /* racer_spray stepped its spray this step */
#define RACER_CLIMBING 0x40 /* its Chao climbs this step */
#define RACER_CHANGED 0x80 /* its Chao's record has a new time or win */
/* the racer leaves the start late. race_starts_fill sets it for a rival whose
 * record has CHAO_BEGG. race_chao_wait then holds the racer at the start until
 * the racer in first place has run race_chao_late_start's distance. the
 * racer's Chao also runs as grown, child or not, and at the goal takes
 * race_chao_ready's motion after 120 steps */
#define RACER_LATE_START 0x100
/* its Chao is held from racing this step: race_chao_step clears it, and the
 * Chao's play, tumble, pause, drown or dance sets it again each step */
#define RACER_HELD 0x200
/* the racer may go for its item. race_guide_racers sends it there while its
 * Chao touches a small zone, and racer_to_item clears the flag */
#define RACER_CURIOUS 0x400
#define RACER_TURNING 0x800 /* its Chao turns toward a target, in place */
#define RACER_SHOWN 0x1000 /* its Chao and its shadow are drawn */
#define RACER_NAMED 0x2000 /* its Chao's name is drawn, once */
/* set by a cheer or a speed boost. race_chao_take_skill_value's next call takes
 * the skill as full, and clears it */
#define RACER_CHEERED 0x4000
/* its path updates skip the first other path while its Chao doesn't swim */
#define RACER_SKIPS_FIRST_PATH 0x8000

/* how a racer's Chao moves, which its mode stores in gait */
enum racer_gait {
    GAIT_FOOT,
    GAIT_FLY,
    GAIT_SWIM,
    GAIT_CLIMB,
    GAIT_WAIT, /* at the start, on land or afloat */
    GAIT_STILL /* at a pose, an item or the finish */
};

/* where race_guide_racers has sent a racer */
enum racer_route {
    ROUTE_START, /* nowhere yet */
    ROUTE_TARGET,
    ROUTE_ITEM
};

/* what eating a coconut leaves a racer with, chosen at random */
enum racer_effect { EFFECT_NONE, EFFECT_DANCE, EFFECT_BOOST };

/* what a racer does at the race's bidding: step, which racer_step calls,
 * and the state and timer it keeps */
struct racer_behavior {
    Uint8 priority;
    Uint8 state;
    Uint16 timer; /* the steps it has run in its state */
    void (*step)(struct racer *racer);
};

/* how far a racer has come with an item's ask */
enum racer_reaction_status {
    REACTION_PENDING, /* not taken up: the ask is new, or there is none */
    REACTION_TAKEN, /* its behavior taken up */
    REACTION_DONE /* its behavior over */
};

/* the item that asked for a behavior, as racer_tell binds it */
struct racer_reaction {
    struct task *source; /* borrowed while bound, or null */
    Sint8 kind; /* the item's index in item_tells, or -1 */
    /* the ask, an ITEM_ message from ITEM_SWING to ITEM_TAKE, or -1 for
     * none */
    Sint8 mode;
    Uint8 status; /* an enum racer_reaction_status */
    Uint8 unk_07;
};

struct racer {
    struct spray spray; /* its Chao's splashes */
    struct racer_behavior behavior;
    struct racer_reaction reaction;
    Uint8 lane;
    Uint8 segment; /* the course section, from 0 to 16 */
    Uint16 flags;
    Uint8 place; /* its place in the race, from 0 for first */
    Uint8 state; /* where it stands in the race, an enum racer_state */
    Uint8 command; /* an enum racer_command, or COMMAND_NONE once taken */
    Uint8 route; /* where it was sent, an enum racer_route */
    Float rise; /* how far its Chao rose this step */
    Float distance; /* how far along the course */
    /* the index in race_paths of the path it follows: 0 or 1 for another
     * path, and 2 for the course or for none */
    Uint8 path_index;
    Uint8 effect; /* what its last coconut left it, an enum racer_effect */
    /* counts its effect's steps from 0. an effect that runs its full time
     * leaves 1, and unless racer_init or a restart clears it first, the next
     * effect counts on from there */
    Uint16 effect_timer;
    /* the weave phase. a running Chao weaves across its heading by the
     * phase's sine */
    Angle phase;
    Uint8 gait; /* how its Chao moves, an enum racer_gait */
    Uint8 run_motion; /* the motion its Chao last took to run or push */
    /* bit 0: a wall its Chao touches leans out past 15 degrees */
    Uint8 overhang;
    Uint8 voice; /* the voice its Chao asks for, 1 to 17, or 0 */
    Uint8 in_zone; /* 1 while its Chao touches a small zone */
    Uint8 unk_e1;
    Uint16 gain_timer; /* the steps of race_chao_add_gains' cycle */
    /* how far its Chao has gone since its last step's sound */
    Float sound_distance;
    /* what race_chao_add_gains next adds to its Chao's sleep, unk_64 and
     * unk_65 */
    Sint8 sleep_gain;
    Sint8 unk_64_gain;
    Sint8 unk_65_gain;
    Uint8 unk_eb;
    struct alife_chao *chao;
    struct task_work *work; /* its Chao's */
};

/* a racing Chao's work keeps its racer in its vars */
struct racer_vars {
    Uint8 unk_00[8];
    struct racer *racer;
};

#define RACER_VARS(work) ((struct racer_vars *)&(work)->vars)

/* a racer takes its Chao in two halves, in either order. racer_init resets
 * it for lane and returns it, and racer_attach gives it the Chao and the
 * Chao's work. once both have run, it has its Chao. racer_init before
 * racer_attach nulls chao and work. racer_detach undoes both halves, nulls
 * chao and work, and leaves the Chao's task alone */
struct racer *racer_init(struct racer *racer, int lane);
void racer_attach(struct racer *racer, struct task_work *work,
                  struct alife_chao *chao);
void racer_detach(struct racer *racer);
/* with chao set, destroy the Chao's task now and clear every flag. chao and
 * work still point at what that freed. with chao null, it does nothing */
void racer_destroy_chao(struct racer *racer);
/* 1 while the racer has its Chao, else 0 */
int racer_has_chao(struct racer *racer);
/* the Chao and its work while the racer has its Chao, else null */
struct alife_chao *racer_chao(struct racer *racer);
struct task_work *racer_work(struct racer *racer);

/* run the racer a step. with its Chao, racer_step first clears RACER_SHOWN
 * while the Chao's work is closer than 5 to camera_place's pos, and sets it
 * otherwise. its state then runs: a state enum racer_state gives a command
 * issues it, and each RACER_TO_ state moves on to the next.
 * RACER_TO_PODIUM also puts the work at race_course.targets[place + 8], and
 * needs the work. RACER_TO_RESTART runs RACER_RESTARTING in the same step.
 *
 * in RACER_RESTARTING, a racer with its Chao puts the work at
 * race_course.starts[kind][lane] and updates its path. it then resets as
 * racer_init resets it, which takes it back to RACER_TO_WAIT. its Chao's
 * task, if it has one, resets with race_chao_reset.
 *
 * then, with its Chao, the racer takes up any item's behavior and runs its
 * behavior's step, in an active state other than RACER_FINISHING. its effect
 * runs a step, and it finds its course section and updates its path. last,
 * unless racer_spray stepped its spray this step, the spray steps with a null
 * pos and speed: the drops in flight fly on, and no waiting drop starts. the
 * spray is then drawn either way */
void racer_step(struct racer *racer, struct race_path *path, int kind);
/* hide or show the Chao, as racer_step does. RACER_TO_ENTRY issues
 * COMMAND_WINDOW and moves to RACER_IN_ENTRY. in either state, the Chao's
 * work goes to its lane's place at z -30, so both need the work. lanes 0 to 3
 * stand at y 5 and lanes 4 to 7 at y -5. each row of four stands at x 10, 5,
 * -5 and -10, in lane order */
void racer_entry_step(struct racer *racer);
/* set DROP_SPRAYING on each drop and step the spray from pos at speed, in
 * place of racer_step's own step of it. racer_draw_spray draws the spray */
void racer_spray(struct racer *racer, NJS_POINT3 *pos, NJS_VECTOR *speed);
void racer_draw_spray(struct racer *racer);
/* each returns 1 in its states, else 0. racer_is_active's are RACER_TO_REST
 * to RACER_RACING, and RACER_FINISHING. racer_at_goal's are RACER_TO_GOAL and
 * RACER_AT_GOAL, and racer_at_podium's RACER_TO_PODIUM and RACER_ON_PODIUM */
int racer_is_active(struct racer *racer);
int racer_at_goal(struct racer *racer);
int racer_at_podium(struct racer *racer);
/* when command is the racer's, take it, leaving COMMAND_NONE, and return
 * 1; else 0 */
int racer_take_command(struct racer *racer, int command);
/* 1 while the race follows this racer's lane, else 0 */
int racer_is_followed(struct racer *racer);
/* end the racer's effect and issue COMMAND_RACE. without an effect, it does
 * nothing */
void racer_clear_effect(struct racer *racer);
/* print with njPrint, from column x and row y, while the racer has its Chao.
 * racer_print_motion prints the lane, the command and the distance. it adds
 * the Chao's wins, mode and submode, position and speed. racer_print_record
 * prints the Chao's type, wins, skills and sleep, and its unk_65 labeled
 * STRESS. while a race runs, racer_print_record adds the Chao's best time in
 * the race's kind */
void racer_print_motion(struct racer *racer, int x, int y);
void racer_print_record(struct racer *racer, int x, int y);
/* its place in the race, from 0 for first */
int racer_place(struct racer *racer);
/* aim the Chao at target: set the Chao's target, clear the racer's weave
 * phase and set RACER_AIMED. aiming needs the Chao. then, while the racer has
 * its Chao, turn the work to face the target */
void racer_face_target(struct racer *racer, NJS_POINT3 *target);
/* set RACER_HOLD_PATH, so that the next path update leaves the racer's
 * target and distance as they are */
void racer_hold_path(struct racer *racer);
/* the index in race_paths of the path its last path update followed, 2 for
 * the course or for none */
int racer_path_index(struct racer *racer);

/* each of these starts a behavior's step at state 0 and timer 0.
 * racer_set_behavior starts step and stores priority only when priority is
 * above the stored one, and then returns 1. otherwise it changes nothing and
 * returns 0. the stored priority is a byte, so a priority above 255 passes
 * the test, and only its low byte is stored. every caller passes 1.
 * racer_override_behavior starts step at the stored priority plus 1, which
 * takes 255 to 0. racer_clear_behavior starts the idle step at priority 0 */
int racer_set_behavior(struct racer_behavior *behavior,
                       void (*step)(struct racer *racer), int priority);
void racer_override_behavior(struct racer_behavior *behavior,
                             void (*step)(struct racer *racer));
void racer_clear_behavior(struct racer_behavior *behavior);

/* racer_go_to's results: still on its way, there, or out of time */
enum racer_progress { PROGRESS_GOING, PROGRESS_DONE, PROGRESS_TIMED_OUT };

/* head for target, from a behavior's step, and count the step in the
 * behavior's timer. at timer 0, issue COMMAND_RETURN. the Chao has arrived
 * once its work is within radius of target across x and z, the edge
 * included. until then, each call aims the Chao at target. arriving in time
 * issues COMMAND_RETURN and takes a new weave phase.
 *
 * return PROGRESS_TIMED_OUT once the count passes timeout, even on arriving.
 * otherwise return PROGRESS_DONE on arriving, and PROGRESS_GOING until then.
 * a timeout of 0 times out at once, and one of 65535 or more never does, as
 * the timer is 16 bits */
int racer_go_to(struct racer *racer, NJS_POINT3 *target, Float radius,
                int timeout);
/* take an item's ask, as item_tell_chao passes it, and return 1 whatever it
 * does. status goes back to REACTION_PENDING first, so while a mode is bound,
 * racer_step takes up its behavior again, one priority higher. ITEM_LET_GO
 * unbinds the reaction only when source and kind are the bound ones: source
 * goes null, and kind and mode -1. ITEM_GO_TO binds the mode alone, and
 * keeps source and kind, null and -1 when none were bound. any other mode
 * binds mode, source and kind. a bound source must outlive the behavior it
 * asks for */
int racer_tell(struct racer *racer, Sint8 mode, struct task *source,
               Sint8 kind);
/* with a source bound, tell it ITEM_LET_GO and clear the reaction. either way,
 * clear the behavior */
void racer_cancel_item(struct racer *racer);
/* which item behavior runs: 1 play, 2 eat, 3 swing, 4 walk to the coconut
 * and 5 push, else 0 */
int racer_item_behavior(struct racer *racer);

#endif

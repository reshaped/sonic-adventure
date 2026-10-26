/* players: the characters in a stage, each a task with a record of its own */
#ifndef PLAYER_PLAYER_H
#define PLAYER_PLAYER_H
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/ground.h"

struct angles;
struct e102;
struct event_mouth_key;
struct path;
struct polygon_ball;
struct task;
struct task_motion;
struct task_work;

#define CHARACTER_SONIC 0
#define CHARACTER_EGGMAN 1
#define CHARACTER_TAILS 2
#define CHARACTER_KNUCKLES 3
#define CHARACTER_TIKAL 4
#define CHARACTER_AMY 5
#define CHARACTER_E102 6
#define CHARACTER_BIG 7
#define CHARACTER_COUNT 8

/* the work ids of a player's task and of the tasks it meets */
#define PLAYER_ID 1 /* a player's, unless its task was made with another */
#define RACER_ID 2 /* the player Tails races */
#define RIVAL_ID 3 /* the rival in a battle */
#define RAMP_ID 9 /* a ramp a board leaps off */

#define PLAYER_HURT_STEPS 120 /* from a hit until it can be hurt again */
#define PLAYER_SPIN_SPEED 1.85F /* below this, a spin along a path uncurls */
#define PLAYER_EDIT_SPEED 5.0F /* how far edit mode flies a player a step */

/* a player task's work->vars, which PLAYER_VARS reads in place. arg holds
 * what the requested submode takes */
struct player_vars {
    Uint8 player; /* its slot: an index into player_work, not a character */
    Uint8 character; /* CHARACTER_* */
    Uint8 unk_02[6]; /* each character spends these its own way */
    union {
        struct task *carry; /* what PLAYER_MODE_HOLD hands it to carry */
        struct path *path; /* for PLAYER_MODE_PATH and the other path modes */
    } arg;
};

#define PLAYER_VARS(work) ((struct player_vars *)&(work)->vars)

/* bits of a player task's work->flags */
#define PLAYER_ON_LAND 0x1 /* on the world's ground, or on a path */
#define PLAYER_ON_TASK 0x2 /* on top of another task's collision */
#define PLAYER_ON_GROUND (PLAYER_ON_LAND | PLAYER_ON_TASK)
#define PLAYER_HURT 0x4 /* collision's COLLISION_DAMAGED: a check hurt it */
#define PLAYER_BALL 0x100 /* curled up into a ball */
/* Sonic's light dash is charged, which shows his aura */
#define SONIC_CHARGED 0x200
#define PLAYER_GUARD 0x400 /* what E-102's shots bounce off */
#define PLAYER_ATTACKING (PLAYER_BALL | PLAYER_GUARD)
#define PLAYER_HOLDING 0x800 /* the player carries record->held */
#define PLAYER_REQUEST 0x1000 /* work->submode holds a requested submode */
#define PLAYER_ON_PATH 0x2000 /* it runs along a path, in a mode of its own */
#define PLAYER_EVENT_DRIVEN 0x4000 /* an event moves it, not the pads */
/* set while it recoils from a hit, by its character's step */
#define PLAYER_RECOILING 0x8000

/* bits of a player record's effects */
#define PLAYER_BARRIER 0x1
#define PLAYER_MAGNETIC_BARRIER 0x2
#define PLAYER_FALLING 0x4000 /* out of the stage */
#define PLAYER_INVINCIBLE 0x8000

/* the items each character picks up, in a player record's upgrades */
#define PLAYER_E102_LASER 0x200 /* changes his arm and his shots */
#define PLAYER_E102_BOOSTER 0x400 /* he hovers with it */
/* Sonic's crystal ring, on his -z wrist, charges his light dash sooner */
#define PLAYER_SONIC_CRYSTAL_RING 0x1
#define PLAYER_SONIC_LIGHT_SHOES 0x2 /* he dashes along rings in them */
/* Sonic's ancient light aims his dash at what he can attack */
#define PLAYER_SONIC_ANCIENT_LIGHT 0x4
#define PLAYER_SONIC_SUPER 0x8000
/* Tails' jet anklet speeds his flight and changes his shoes */
#define PLAYER_TAILS_JET_ANKLET 0x8
#define PLAYER_TAILS_RHYTHM_BROOCH 0x10 /* keeps his tail attack going */
/* Knuckles' fighting gloves let him charge his maximum heat and change all
 * his hands. his shovel claws let him dig, and change two of his hands */
#define PLAYER_KNUCKLES_FIGHTING_GLOVES 0x20
#define PLAYER_KNUCKLES_SHOVEL_CLAWS 0x40
#define PLAYER_BIG_POWER_ROD 0x800 /* changes his rod's model */
#define PLAYER_BIG_LIFE_BELT 0x1000 /* floats him in water */
/* Amy's long hammer changes her hammer and the sound it hits with */
#define PLAYER_AMY_LONG_HAMMER 0x80
/* Amy's warrior feather, drawn on her head, lets her swing go on into a
 * spin while the button is held */
#define PLAYER_AMY_WARRIOR_FEATHER 0x100

/* a player_anim's state while an action starts. once it has started, the
 * state is the action's mode */
#define PLAYER_ANIM_NEW 0 /* anim->blend isn't made yet */
#define PLAYER_ANIM_CHANGE 1 /* another action is asked for */
#define PLAYER_ANIM_BLENDING 2 /* blending into the new action */
/* the modes, which say how an action plays. next is the action its entry
 * names to play after it */
#define PLAYER_ANIM_LOOP 3 /* round and round */
#define PLAYER_ANIM_ONCE 4 /* once, then next */
#define PLAYER_ANIM_HOLD 5 /* once, holding its last frame */
#define PLAYER_ANIM_BACK 6 /* PLAYER_ANIM_ONCE, backward */
#define PLAYER_ANIM_BACK_HOLD 7 /* PLAYER_ANIM_HOLD, backward */
#define PLAYER_ANIM_BLEND_NEXT 8 /* once, then blending into next */
#define PLAYER_ANIM_LOOP_SPEED 9 /* round, paced by the speed ahead */
#define PLAYER_ANIM_LOOP_PACE 10 /* round, paced by action_speed */
/* once, action_speed frames a step, then next */
#define PLAYER_ANIM_ONCE_PACE 11
#define PLAYER_ANIM_DRIVEN 12 /* at whatever frame its player sets */
#define PLAYER_ANIM_FREE 14 /* free anim->blend, as the player goes */
/* in a player_anim's flags, set on the update a PLAYER_ANIM_ONCE,
 * PLAYER_ANIM_ONCE_PACE or PLAYER_ANIM_BACK action ends. it is set on every
 * update PLAYER_ANIM_HOLD or PLAYER_ANIM_BACK_HOLD holds its end frame, and
 * never for PLAYER_ANIM_BLEND_NEXT */
#define PLAYER_ACTION_DONE 0x1

/* how a player's model plays its character's actions and welds its seams */
struct player_anim {
    Sint16 state; /* a PLAYER_ANIM_* state or mode */
    Sint16 weld_mode; /* a PLAYER_WELD_* mode */
    Uint16 action; /* the index in actions asked for */
    Uint16 current; /* the index accepted, as blending starts */
    Uint16 previous; /* current as the last player_anim_step began */
    Uint16 next; /* the action after current, from its entry */
    Sint16 updates; /* since a change, or a blend into next */
    Sint16 flags; /* PLAYER_ACTION_DONE, cleared on each update */
    Float frame; /* the blend's share while blending, else the frame shown */
    /* the frame a looping or driven action starts on, and then 0 */
    Float start_frame;
    Float *speed; /* the record's speed ahead, for PLAYER_ANIM_LOOP_SPEED */
    Float *pace; /* the record's action_speed, for the two _PACE modes */
    struct player_action *actions; /* the character's table, borrowed */
    struct weld *welds; /* a writable table, borrowed, ended by a null base */
    /* the action anim plays while it blends, which it allocates and
     * PLAYER_ANIM_FREE frees */
    NJS_ACTION *blend;
};

/* the bytes of a player's record that each character spends its own way,
 * like task_vars in a task */
union character_vars {
    Sint8 b[32];
    Sint16 w[16];
    Sint32 l[8];
    Float f[8];
    void *p[8];
    struct {
        /* @hack type: E-102's record in an array of one, whose address the
         * image forms with ADD #124 (e102_step at 0x8c0d4834), where a plain
         * pointer loads its offset with MOV #124,R0; as a pointer, summed
         * over the 15 functions that change in e102.c, e102_effects.c,
         * e102_mode.c and rival_e102.c, 707 of 8576 words differ */
        struct e102 *record[1];
        /* the steps he has kept his running speed, which on the 132nd start
         * his skate */
        Sint16 skate_steps;
    } e102;
    struct {
        /* a bit for each step Sonic dashed in, the lowest for this step */
        /* @hack type: an array of one, whose address the image forms with
         * ADD #124 (sonic_step at 0x8c0adfd0), where a plain Sint8 loads its
         * offset with MOV #124,R0; as a Sint8, summed over sonic_step,
         * effects.c's dash_step and super.c's super_dash_step, 342 of 3550
         * words differ */
        Sint8 dashes[1];
        Uint8 unk_01;
        Sint16 curled_steps;
        /* the steps left of a light dash, or those a homing attack has flown
         * without a target */
        Sint16 dash_steps;
        Sint16 steps; /* in his mode */
        Uint8 unk_08[0x10];
        /* how far the model of each toe blends, on his +z side and his -z
         * side */
        Float toe_blend_plus_z;
        Float toe_blend_minus_z;
    } sonic;
    struct {
        /* the ends of his tails as his tail attack last drew them */
        /* @hack type: the ends as a union of six floats and two points, read a
         * float at a time through coord, for which SHC forms coord's address
         * with ADD #124 (tails_try_tail_attack at 0x8c0c53e8). as two points
         * alone, read by their fields, summed over tails_try_tail_attack,
         * tails_attack_trails and tails_set_shapes, tails.c's code runs 32
         * bytes short and 624 of 760 words differ */
        union {
            Float coord[6];
            NJS_POINT3 point[2];
        } tips;
        Float spin; /* how fast his tails spin, from 0 to 1 */
        Float flight; /* how much of his flight he has spent, from 0 to 1 */
    } tails;
    union {
        struct {
            Uint8 unk_00[5];
            /* how often his glide bounced off a wall lately: knuckles.c adds
             * 20 for each bounce and counts it down */
            Sint8 bumps;
        } glide;
        /* words knuckles.c reads by index */
        /* @hack type: words read by index, whose addresses the image forms
         * in a register (ADD #124 and ADD #10 for w[5] at 0x8c0ccb34), where
         * a named field loads its offset into R0; as named fields, summed
         * over the four functions of knuckles.c that change, 226 of 7035
         * words differ */
        Sint16 w[7];
    } knuckles;
};

/* a target a falling player can steer onto, as player_add_target lists it */
struct player_target {
    NJS_POINT3 pos;
    Float radius;
    Float distance; /* from the player to the target's surface */
    Float across; /* from the player to the target's middle, leaving out y */
    Angle off; /* the turn to the target, from the way the player faces */
    Angle ang; /* the heading from the player to the target */
};

#define PLAYER_TARGETS 15 /* how many a player's record has room for */

/* a character's constants, which player_init copies into its record */
struct character_constants {
    Sint32 jump_hold_steps; /* how long a held jump can keep growing */
    /* how far down its collision ball reaches, below where it stands, to
     * keep it on the ground */
    Float snap;
    Float speed_limit; /* the fastest it moves along the world's x or z */
    Float fall_limit; /* the fastest it moves along the world's y */
    /* the stick speeds it up only to this, and resistance grows past it */
    Float top_speed;
    Float push_speed; /* the fastest it pushes or pulls what it touches */
    Float jump_speed; /* its upward speed as a jump starts */
    Float launch_speed; /* launched up faster than this, it can't steer */
    Float slide_speed; /* down a steep slope, faster than this, it slides */
    /* the speeds each faster run starts at */
    Float trot_speed;
    Float running_speed;
    Float sprint_speed;
    /* this fast or faster, it crashes into a wall it runs at */
    Float crash_speed;
    /* the top tier of a run, past sprint_speed. along a path, the stick
     * brings the player up to it. faster than it, the player turns more
     * slowly, and player_turn_upright leaves it as it is */
    Float full_speed;
    /* added to its upward speed, scaled, each step a jump is held */
    Float jump_hold_speed;
    /* what's added to its speed a step: by the stick, pushed all the way,
     * on the ground and in the air */
    Float ground_accel;
    Float air_accel;
    Float ground_drag; /* on the ground once the stick is let go, below zero */
    Float brake_drag; /* against its speed, as it brakes */
    Float air_brake; /* against its speed in the air, the stick pointing back */
    /* resistance, as negative factors of the speed it acts against, added
     * each step */
    Float air_resist; /* of the speed ahead, in the air */
    NJS_VECTOR resist; /* of each axis of the speed, in the frame it faces */
    /* the ground's grip, as negative factors of the player's push into the
     * ground */
    Float moving_grip; /* caps a push back, on WORLD_GRIP_LEAST ground */
    Float side_grip; /* against its speed across */
    Float still_grip; /* holds a still axis still, and caps a push ahead */
    Float unk_6c;
    Float radius; /* of its collision shape */
    Float height; /* the character's, from its feet */
    Float gravity; /* the speed it gains each step, along world_gravity */
    Float eye_height; /* from its feet */
    Float center_y; /* how high its middle is, from its feet */
};

/* the steps a player's eyes stay under liquid before it starts to drown */
#define PLAYER_DROWN_STEPS 1080

/* a player's own record, beside its task */
struct player {
    Float action_speed; /* the pace of actions in the _PACE modes */
    Sint16 upgrades; /* as the story gives them */
    Sint16 effects; /* PLAYER_BARRIER and the other effect bits */
    Sint16 jump_steps; /* how long jump is held, from 0 as a jump starts */
    Sint16 launch_steps; /* left of a launch, beyond its control */
    /* the steps its eyes have spent under liquid, back to 0 only once no
     * liquid is above it */
    Sint16 underwater_steps;
    Sint16 idle_steps; /* how long it has stood still */
    Sint16 last_flags; /* its work flags as the last collision began */
    Sint16 target_count; /* of its targets, from 0 each step */
    Sint16 path_point; /* where it hangs on its path */
    Uint8 unk_16[2];
    Float path_distance; /* how far along its path it is */
    /* how level the ground it stands on is, against gravity: 1 when level,
     * 0 for a wall, and below 0 under a ceiling */
    Float level;
    NJS_VECTOR nudge; /* added to its speed in the world for a step */
    /* accel and speed are in the frame the player faces, x ahead */
    NJS_VECTOR accel; /* what speed gains this step */
    NJS_VECTOR speed;
    NJS_VECTOR wall_normal; /* of the wall it last touched */
    NJS_VECTOR ground_normal; /* of the ground it stands on */
    Uint32 ground_flags; /* the world flags of that ground */
    Uint32 last_ground_flags; /* ground_flags, the step before */
    struct polygon_ball *contact; /* the ball the collision sweeps */
    struct task *held; /* what it carries */
    struct task *ground_task; /* the task whose object it stands on */
    /* the player_mouth of its mouth task, set on that task's first step.
     * nothing clears it, so it dangles once the task ends */
    struct player_mouth *mouth;
    NJS_OBJECT *ground_object; /* the object of the ground it stands on */
    struct player_target *targets; /* room for PLAYER_TARGETS */
    union character_vars own; /* what each character keeps of its own */
    struct character_constants consts;
    struct player_anim anim;
    /* where each WELD_MARK weld keeps its point and its axis: weld_point0
     * and weld_axis0 for WELD_MARK, on to weld_point5 and weld_axis5 for
     * WELD_MARK + 5 */
    NJS_POINT3 weld_point0;
    NJS_POINT3 weld_point1;
    NJS_POINT3 weld_point2;
    NJS_POINT3 weld_point3;
    NJS_POINT3 weld_point4;
    NJS_POINT3 weld_point5;
    NJS_VECTOR weld_axis0;
    NJS_VECTOR weld_axis1;
    NJS_VECTOR weld_axis2;
    NJS_VECTOR weld_axis3;
    NJS_VECTOR weld_axis4;
    NJS_VECTOR weld_axis5;
    struct shadow shadow; /* what's above and below it, such as water */
};

/* a player's weld_mode: what becomes of the vertices of its welds */
#define PLAYER_WELD_MAKE 0 /* made on the next pass */
#define PLAYER_WELDING 1 /* kept as the player is posed */
#define PLAYER_WELD_FREE 2

/* a weld of a player's model, which closes a seam as the model is posed:
 * vertices the two objects share move together */
struct weld {
    NJS_OBJECT *base; /* the model it belongs to, or null to end a table */
    NJS_OBJECT *object_a;
    NJS_OBJECT *object_b;
    Uint8 count; /* of shared vertices, or a WELD_MARK weld's axis */
    Sint8 kind; /* a weld_kind */
    Uint8 node_a; /* each object's node in the model */
    Uint8 node_b;
    NJS_POINT3 *vertices; /* for each: a's point and normal, then b's */
    Uint16 *indices; /* for each: its vertex in a, then in b */
};

/* what a weld does, by its kind */
enum weld_kind {
    WELD_OFF, /* nothing */
    WELD_BOTH, /* both objects' vertices meet halfway */
    WELD_TO_A, /* b's follow a's */
    WELD_TO_B, /* a's follow b's */
    /* WELD_MARK to WELD_MARK + 5 have no object_b. each keeps object_a's
     * node's origin, as posed, in the record's weld_point of its number. it
     * keeps one axis of that node in the weld_axis of its number: x, y or z
     * as count is 0, 1, or more */
    WELD_MARK
};

/* an action in a stage's module, at the address it loads at */
#define MODULE_ACTION(address) ((NJS_ACTION *)(address))

/* an entry in a character's table of actions */
struct player_action {
    NJS_ACTION *action;
    Uint8 nodes; /* in its model */
    Sint8 mode; /* how it plays, for its anim state */
    Uint16 next; /* the action after it, for those that end */
    Float blend; /* how much of the way it blends in a step: 1 at once */
    Float speed; /* the frames it plays a step */
};

/* how a player turns its head and eyes toward what it looks at: where each
 * aims, the angles they have, and how far each may turn */
struct player_look {
    Angle head_aim_y, head_aim_z;
    Angle head_y, head_z;
    Angle eyes_aim_y, eyes_aim_z;
    Angle eye0_y, eye0_z;
    Angle eye1_y, eye1_z;
    Angle head_max_y, head_min_y;
    Angle head_max_z, head_min_z;
    Angle eye0_max_y, eye0_min_y;
    Angle eye0_max_z, eye0_min_z;
    Angle eye1_max_y, eye1_min_y;
    Angle eyes_max_z, eyes_min_z;
    NJS_OBJECT *head; /* the model's nodes it turns */
    NJS_OBJECT *eye0;
    NJS_OBJECT *eye1;
};

/* a look task's mode: it starts, turns the head ahead, at the camera, or
 * as an event says, and ends once its player has gone */
enum look_mode { LOOK_INIT, LOOK_AHEAD, LOOK_CAMERA, LOOK_EVENT, LOOK_END };

/* an eye's own angles, as its model had them, or for the eyes that move
 * rather than turn, its place */
union player_look_eye {
    Angle ang[3];
    Float pos[3];
};

/* what a look task keeps in its motion: what the turns need, and the
 * nodes as the model had them, to put back as the look ends */
struct player_look_motion {
    struct player_look look;
    Angle head_ang[3];
    union player_look_eye eye0;
    union player_look_eye eye1;
};

#define PLAYER_LOOK(task) ((struct player_look_motion *)(task)->motion)

/* what the task that sways a player's head keeps in its motion */
struct player_sway {
    Uint8 unk_00[8];
    Float bob; /* how far the head bobs as its player rises and falls */
    Float sway; /* how far the wind or the player's speed sways it */
    Uint8 unk_10[0x18];
    Angle lean; /* the way the sway leans */
    Angle sway_phase;
    Angle bob_phase;
};

#define PLAYER_SWAY(task) ((struct player_sway *)(task)->motion)

/* a sway task's mode: it starts, sways, and ends once its player has gone */
enum sway_mode { SWAY_INIT, SWAY_SWAY, SWAY_END };

/* a player's mouth, which moves through the shapes of its motion as the
 * player talks. it hangs from the face as the face's sibling, in its own
 * shape or a blend of two */
struct player_mouth {
    Sint8 from; /* the shapes it blends between */
    Sint8 to;
    Uint8 unk_02;
    Sint8 key; /* which key of the script it plays */
    Float amount; /* of the way from one shape to the other */
    Float step; /* added to amount each step */
    struct event_mouth_key *script; /* to play, or null */
    NJS_OBJECT *face; /* the object the mouth hangs from, as its sibling */
    NJS_OBJECT *mouth; /* the mouth's own shape */
    NJS_OBJECT *blend; /* the shape it blends to */
    NJS_MOTION *shapes;
};

/* a mouth task's mode: it starts, and then stays still, moves on to the
 * next key of its script, plays the key, or moves with the letters of its
 * player's text */
enum mouth_mode {
    MOUTH_INIT,
    MOUTH_STILL,
    MOUTH_NEXT_KEY,
    MOUTH_SCRIPT,
    MOUTH_TALK
};

/* the slots for players. player_work, player_motion, player_tasks and
 * player_records hold one for each, as the history does */
#define PLAYER_COUNT 8

/* a zone a player goes down in: an object, and a bit for each character
 * it takes */
struct death_zone {
    Uint32 characters;
    NJS_OBJECT *object;
};

/* each stage's lists of zones, by act, each ended by a null object */
extern struct death_zone **death_zones[];

/* each slot's task work and motion, null while the slot is empty. the
 * motion's record is the player's record */
extern struct task_work *player_work[];
extern struct task_motion *player_motion[];
/* each slot's task and record, and its history: where the player was and
 * how it faced each step, the latest at player_history_index. only player.c
 * reads the tasks and the history. they are declared here because SHC lays
 * out a file's zeroed globals in the order it first meets them */
#define PLAYER_HISTORY_STEPS 256
extern struct task *player_tasks[PLAYER_COUNT];
extern struct player *player_records[PLAYER_COUNT];
extern Uint8 player_history_index[PLAYER_COUNT];
extern NJS_POINT3 player_history_pos[PLAYER_HISTORY_STEPS][PLAYER_COUNT];
extern Angle player_history_ang[PLAYER_HISTORY_STEPS][PLAYER_COUNT][3];
/* the kind of thing each character holds, by character. a field's holdable
 * object sets it while it's held and clears it once it's let go. the Chao
 * garden reads it to put the egg it names in the player's hands */
extern Sint8 player_held_kinds[CHARACTER_COUNT];

/* a player argument is a slot, in [0, PLAYER_COUNT), not a CHARACTER_*.
 * only the player_give_ functions test its range. the slot must hold a
 * player, unless a comment says how an empty one is taken. a task or record
 * pointer returned is borrowed, and valid until player_free empties its
 * slot */

/* set every slot's task, work, motion and record to null */
void player_clear_slots(void);
/* TRUE as the player's pad presses one of pad_debug_buttons while
 * debug_unlocked is set, else FALSE */
Bool player_debug_pressed(int player);
/* make task the player in slot player, whose work's vars must hold its
 * character. player_init gives it a record with that character's constants
 * and upgrades, adding Tails' upgrades for Sonic and Big's for E-102, and
 * fills the slot. player_free empties the slot and frees the record, with
 * its ball, blend, weld vertices and targets */
void player_init(int player, struct task *task);
void player_free(int player);
/* end the player's step:
 * - clear PLAYER_REQUEST and COLLISION_TOUCHED in work->flags
 * - zero motion->accel, motion->slip, record->nudge, record->accel and the
 *   target count
 * - count launch_steps down to 0
 * - note where the player is and how it faces in its history
 * - count underwater_steps up on each step liquid's surface is above the
 *   player's eyes, starting a drown task as it reaches PLAYER_DROWN_STEPS,
 *   and set it to 0 once no liquid is above the player */
void player_finish_step(struct task_work *work, struct task_motion *motion,
                        struct player *record);
/* the work id of the task the player touches side on, on a step it presses
 * one of its action buttons, else 0. it's 0 while the player is hurt */
int player_action_touch_id(struct task_work *work);
/* player_carry_flags' bits: the player may carry the task, push and pull
 * it, or shake it */
#define PLAYER_CAN_CARRY 0x200
#define PLAYER_CAN_PUSH 0x400
#define PLAYER_CAN_SHAKE 0x800
/* the PLAYER_CAN_* bits for the task the player carries, or else for the
 * holdable task it touches side on. the bits follow the task's carry kind,
 * its work id.
 * the result is 0 for no such task, and for a task held already or of no
 * carry kind. it is 0 too for Eggman and Tikal, and while the player is
 * hurt and holds nothing */
int player_carry_flags(struct task_work *work);
/* TRUE while the task the player's collision hit is one to shake, by its
 * carry kind, else FALSE */
Bool player_touches_shakable(struct task_work *work, struct task_motion *motion,
                             struct player *record);
/* push the task the player shakes away from it, hardest at the middle frame
 * of the player's action */
void player_shake_touched(struct task_work *work, struct task_motion *motion,
                          struct player *record);
/* turn the player toward the task it touches. while the task's middle is
 * beyond the reach its carry kind gives, also move the player toward it by
 * the hit normal */
void player_approach_touched(struct task_work *work,
                             struct task_motion *motion);
/* TRUE while the player's up, turned by its angles and then by the current
 * matrix, points up, else FALSE */
Bool player_upright(struct task_work *work);
/* TRUE while the stick is pushed, with *ang and *stroke set to the stick's
 * unless they're null, else FALSE. FALSE leaves *ang and *stroke as they
 * were. the first of these that holds decides instead:
 * - while an event drives the player, the result is whether the event's
 *   record holds an action, and *ang and *stroke are left as they were
 * - while a launch counts down off a path, a pushed stick gives FALSE
 * - with the stick let go, Sonic curled up on Casinopolis' pinball tables,
 *   acts 2 and 3, gets TRUE, with *ang his heading and *stroke 0 */
Bool player_read_stick(struct task_work *work, Angle *ang, Float *stroke);
/* while an event drives the player, TRUE once the event's action is done.
 * the player's speed and the action then clear, and the event lets it go.
 * with no event, TRUE off a path while the stick points more than 135
 * degrees from the player's heading. FALSE otherwise */
Bool player_stick_back_or_end_event(struct task_work *work,
                                    struct task_motion *motion);
/* whether jump is pressed this step: PLAYER_JUMP_PANEL while the player
 * stands on a panel it can jump from, as panel_jump_ready_or_idle says, else
 * PLAYER_JUMP_PRESSED. PLAYER_JUMP_NONE while jump isn't pressed or an
 * event drives the player */
enum player_jump { PLAYER_JUMP_NONE, PLAYER_JUMP_PRESSED, PLAYER_JUMP_PANEL };
enum player_jump player_jump_pressed(struct task_work *work,
                                     struct player *record);
/* TRUE while jump is held and record->jump_steps is below the character's
 * jump_hold_steps, else FALSE. each call with jump held counts jump_steps
 * up. FALSE while an event drives the player */
Bool player_count_jump_hold(struct task_work *work, struct player *record);
/* how far a direction turns from straight into the wall the player touches:
 * player_face_wall's heading, player_stick_into_wall's stick, or
 * player_move_into_wall's motion */
enum player_wall {
    /* from player_stick_into_wall, while the stick is let go or an event
     * drives the player */
    PLAYER_WALL_NO_STICK,
    PLAYER_WALL_AWAY, /* more than 135 degrees */
    PLAYER_WALL_ACROSS, /* more than 73.125 degrees */
    /* more than 39.375 degrees for the heading, or 28.125 for the stick and
     * the motion */
    PLAYER_WALL_ASKEW,
    PLAYER_WALL_HEAD_ON
};
int player_face_wall(struct task_work *work, struct task_motion *motion);
int player_stick_into_wall(struct task_work *work, struct task_motion *motion);
int player_move_into_wall(struct task_work *work, struct task_motion *motion);
/* TRUE while a task is near the player to aim at, in near_tasks for player
 * 0 and in near_tasks_player1 for any other, else FALSE */
Bool player_has_target(Uint8 player);
/* turn the player toward the nearest of those tasks, or by the stick while
 * there's none */
void player_turn_to_target(struct task_work *work, struct task_motion *motion,
                           struct player *record);
/* set record->accel, what the player's speed gains this step, by how it
 * moves. the eight functions below are for moving on the ground, in the
 * air, along a path, pushing or pulling what it touches, braking, launched
 * beyond its control, rolling, and by gravity alone. while an event drives
 * the player, player_accel_ground and player_accel_air follow the event's
 * action, and the other six do nothing */
void player_accel_ground(struct task_work *work, struct task_motion *motion,
                         struct player *record);
void player_accel_air(struct task_work *work, struct task_motion *motion,
                      struct player *record);
void player_accel_path(struct task_work *work, struct task_motion *motion,
                       struct player *record);
void player_accel_push(struct task_work *work, struct task_motion *motion,
                       struct player *record);
void player_accel_brake(struct task_work *work, struct task_motion *motion,
                        struct player *record);
void player_accel_launched(struct task_work *work, struct task_motion *motion,
                           struct player *record);
void player_accel_roll(struct task_work *work, struct task_motion *motion,
                       struct player *record);
void player_accel_gravity(struct task_work *work, struct task_motion *motion,
                          struct player *record);
/* player_accel_wall runs the player down a wall, gravity pulling along the
 * world's -x as well as down. player_accel_fly flies as Tails does: the
 * stick speeds him only within 22.5 degrees of his heading */
void player_accel_wall(struct task_work *work, struct task_motion *motion,
                       struct player *record);
void player_accel_fly(struct task_work *work, struct task_motion *motion,
                      struct player *record);
/* player_accel_glide glides as Knuckles does. it sets motion->accel.y to
 * 0.95 of his gravity, against world_gravity's pull, and takes half the
 * resistance. player_accel_climb sets record->accel to the stick's push
 * alone */
void player_accel_glide(struct task_work *work, struct task_motion *motion,
                        struct player *record);
void player_accel_climb(struct task_work *work, struct task_motion *motion,
                        struct player *record);
/* the stick drives the player: pushed up, it speeds the player ahead by the
 * square of the push, and pushed across, it turns the player */
void player_accel_drive(struct task_work *work, struct task_motion *motion,
                        struct player *record);
/* run the player down the wall as player_accel_wall does, but with the
 * stick and the ground's grip ignored. past top_speed, its resistance is
 * twice resist.x times the excess. below top_speed, the push is 0.3 of
 * ground_accel */
void player_accel_wall_idle(struct task_work *work, struct task_motion *motion,
                            struct player *record);
#define PLAYER_BOARD_SPEED 4.6F /* a board's tricks are judged against it */
/* the max_speed Sonic and Tails board at: the float just below 5.98F */
#define PLAYER_BOARD_TOP_SPEED 5.9799995F
/* in the work flags of the board Sonic or Tails rides, which others set */
#define BOARD_LOWER 0x2 /* it draws lower under him */
#define BOARD_STILL 0x8 /* it stays put, neither falling nor following him */
/* on a board, along the ground. past max_speed, resistance slows the
 * player */
void player_accel_board(struct task_work *work, struct task_motion *motion,
                        struct player *record, Float max_speed);
/* on a board: in the air, in a trick off a ramp, and braking hard or crashed */
void player_accel_board_air(struct task_work *work, struct task_motion *motion,
                            struct player *record);
void player_accel_board_trick(struct task_work *work,
                              struct task_motion *motion,
                              struct player *record);
void player_accel_board_brake(struct task_work *work,
                              struct task_motion *motion,
                              struct player *record);
/* gravity alone moves the player down the slope it stands on, and it faces
 * the way it moves. it sets motion->speed and record->speed itself, and
 * does nothing during a launch */
void player_accel_slope(struct task_work *work, struct task_motion *motion,
                        struct player *record);
/* the snow or sand a board throws up, by the ground under it, and its sound */
void player_board_spray(struct task_work *work, struct task_motion *motion,
                        struct player *record);
/* aim the player the way the stick points, or down the slope it stands on
 * once the stick is let go, and turn it toward its aim */
void player_turn_to_stick(struct task_work *work, struct task_motion *motion,
                          struct player *record);
/* turn the player toward world_gravity's upright, 11.25 degrees a step,
 * keeping its speed in the world. faster than its full speed, it stays as
 * it is, unless it's Super Sonic */
void player_turn_upright(struct task_work *work, struct task_motion *motion,
                         struct player *record);
/* add record->accel to record->speed, and set motion->speed to that speed in
 * the world. the five functions below are for moving free, along a path,
 * pushing what the player touches, climbing and gliding. climbing takes
 * record->accel as the speed */
void player_apply_accel(struct task_work *work, struct task_motion *motion,
                        struct player *record);
void player_apply_path_accel(struct task_work *work, struct task_motion *motion,
                             struct player *record);
void player_apply_push(struct task_work *work, struct task_motion *motion,
                       struct player *record);
void player_apply_climb_accel(struct task_work *work,
                              struct task_motion *motion,
                              struct player *record);
void player_apply_glide_accel(struct task_work *work,
                              struct task_motion *motion,
                              struct player *record);
/* player_collide's results besides 0 and 1 */
#define PLAYER_CRASHED 2
#define PLAYER_TEETERING 3
/* move the player against the walls and onto the ground. it returns 1 as
 * the player runs into a wall or a task's pushing shape, and PLAYER_CRASHED
 * as it crashes into one. it returns PLAYER_TEETERING as an ottotto box it
 * runs onto, facing the box's way, stops it, and 0 otherwise. a slide along
 * a wall can make it return another value instead: an angle, or 12 on
 * Casinopolis' pinball tables */
int player_collide(struct task_work *work, struct task_motion *motion,
                   struct player *record);
/* player_collide as Knuckles climbs a wall: 1 as he runs into a task, or
 * PLAYER_CRASHED at his running speed or faster, and otherwise 0 */
int player_collide_climb(struct task_work *work, struct task_motion *motion,
                         struct player *record);
/* player_collide without stepping up or sliding along the walls: the player
 * lands on the ground and stops at the walls */
int player_collide_simple(struct task_work *work, struct task_motion *motion,
                          struct player *record);
/* move the player along the path it runs on, the way it aims, turned the
 * way the path runs, and set PLAYER_ON_PATH and PLAYER_ON_LAND. off either
 * end, it moves free by motion->speed, with those flags clear */
void player_move_on_path(struct task_work *work, struct task_motion *motion,
                         struct player *record);
/* move the player along the path by its speed alone as it hangs from a
 * ledge, keeping its angles and stopping short of either end. TRUE while
 * it's on the path. off it, the player moves free by motion->speed, and the
 * result is FALSE */
Bool player_shift_on_path(struct task_work *work, struct task_motion *motion,
                          struct player *record);
/* move the player along the path by its speed, the way it aims, turned
 * the way the path runs and stopping short of either end. TRUE while it's
 * on the path. off it, the player moves free by motion->speed, and the
 * result is FALSE */
Bool player_slide_on_path(struct task_work *work, struct task_motion *motion,
                          struct player *record);
/* player_collide and player_take_speed for a player whose ground is the
 * wall it runs down. player_take_wall_speed also keeps the player's x and z
 * within bounds that change with its y, down to y -19000.
 * player_turn_to_wall turns it a step toward standing on that wall, unless
 * it's faster than its full speed */
int player_collide_wall(struct task_work *work, struct task_motion *motion,
                        struct player *record);
void player_take_wall_speed(struct task_work *work, struct task_motion *motion,
                            struct player *record);
void player_turn_to_wall(struct task_work *work, struct task_motion *motion,
                         struct player *record);
/* turn vector from the frame the player faces into the world */
void player_turn_to_world(struct task_work *work, NJS_VECTOR *vector);
/* set record->speed from motion->speed, turned into the frame the player
 * faces. it zeroes small speeds, and y on the ground, keeps the angles to
 * 16 bits, and sets record->level */
void player_take_speed(struct task_work *work, struct task_motion *motion,
                       struct player *record);
/* move the task the player pushes by motion->speed, and set record->speed
 * from motion->speed, turned into the frame the player faces. an x or z
 * within 0.001 of 0 is zeroed, and on the ground y is too */
void player_take_push_speed(struct task_work *work, struct task_motion *motion,
                            struct player *record);
/* the most nodes a blend holds: anim->blend's motion has room for this many */
#define PLAYER_BLEND_NODES 111
/* advance anim by one update. its actions and their models must stay
 * loaded. nothing checks the indices, the node counts, which must not pass
 * PLAYER_BLEND_NODES, or the allocations. step PLAYER_ANIM_FREE before
 * freeing the record that holds anim */
void player_anim_step(struct player_anim *anim);
/* make, keep or free the vertices of the player's welds, by weld_mode */
void player_update_welds(struct task_work *work, struct task_motion *motion,
                         struct player *record);
/* turn the head toward its aim, and the eyes toward theirs, each within
 * its limits */
void player_look_turn_head(struct player_look *look);
void player_look_turn_eyes(struct player_look *look);
/* a mouth task's step, and its destructor, which puts the mouth's own
 * shape back. the task keeps its player_mouth in its motion */
void player_mouth_step(struct task *task);
void player_mouth_destroy(struct task *task);
/* set the player's mouth to play event_mouth_scripts[script], or to stop.
 * a player with no record or no mouth is left alone */
void player_mouth_set_script(Sint8 player, int script);
void player_mouth_stop(Uint8 player);
/* play se at the player for as long as it stays in mode */
void player_sound_in_mode(int se, Uint8 player, Sint16 mode);
/* the pieces that burst from the player as it crashes into a wall */
void player_crash_burst(struct task_work *work);
/* the step of the stage's task that watches player 0 for the act's death
 * zones, and starts a fall out of the stage when one takes the player */
void death_zone_step(struct task *task);
/* a fall out of the stage, as a stage's own objects ask for it */
void player_fall(Uint8 player);
/* request PLAYER_MODE_DOWN and mark the player PLAYER_FALLING, unless it's
 * player 1 or the set editor is open. the act ends on the fall task's 122nd
 * step */
void player_down(int player);
/* turn the player to heading ang, as player_set_angles takes it, and clear
 * its event's action */
void player_set_heading(Sint8 player, Angle ang);
/* set the action of the player's event, which then drives the player.
 * player_event_run runs the player to the point x, z. player_event_move
 * moves it along x, z at a speed of its length. player_event_push pushes it
 * by x, z each step. an empty slot is left alone */
void player_event_run(Uint8 player, Float x, Float z);
void player_event_move(Uint8 player, Float x, Float z);
void player_event_push(Uint8 player, Float x, Float z);
/* player_event_stop marks the event's action done and the player event
 * driven, so that player_stick_back_or_end_event stops the player and lets it
 * go. it leaves an empty slot alone. player_event_end clears the action, and
 * leaves PLAYER_EVENT_DRIVEN as it is */
void player_event_stop(Uint8 player);
void player_event_end(Uint8 player);
/* zero record->speed and motion->speed, either of which may be null */
void player_clear_speed(struct task_motion *motion, struct player *record);
/* turn vector from the world into the frame the player faces */
void player_turn_back(struct task_work *work, NJS_VECTOR *vector);
/* a ripple at pos on 15 percent of calls, while the player moves and pos is
 * no higher than the top of the player. ang is unused */
void player_make_ripple(Uint8 player, NJS_POINT3 *pos, Angle *ang);
/* a splash where the task is, as it goes under water */
void player_make_splash(struct task_work *work);
/* dust from the player's feet as it runs fast */
void player_kick_dust(struct task_work *work, struct task_motion *motion,
                      struct player *record);
/* which of players 0 and 1, counted from 1, is within radius of pos: 0 for
 * neither */
int player_in_range(const NJS_POINT3 *pos, Float radius);
/* 1 while player 0, which must exist, is within radius of pos across and
 * height above or below it, else 0 */
int player_in_cylinder(NJS_POINT3 *pos, Float radius, Float height);
/* push player 0's first shape out of work's first shape, centered on
 * work->pos, until the two overlap by 0.2 */
void player_push_out_of(struct task_work *work);
/* the character the player is, or the game's while it has no work */
int player_get_character(Uint8 player);
/* the slot's task, null for an empty slot */
struct task *player_get_task(Uint8 player);
/* the character of the player whose task it is, or -1 for no player's */
int player_task_character(struct task *task);
/* the slot's record, null for an empty slot */
struct player *player_get(Uint8 player);
/* how many slots hold a player */
int player_count(void);
/* the number of the player nearest pos, or 0 with no player */
int player_nearest(NJS_POINT3 *pos);
/* the first slot on the other side: a rival's, for any player but a rival,
 * and a player's, for a rival. -1 when there's none */
int player_find_rival(Uint8 player);
/* TRUE while the player is on land and its ground task is task, else
 * FALSE */
Bool player_stands_on(Uint8 player, struct task *task);
/* stop the player's task, its step and its draw, and start it again. an
 * empty slot is left alone */
void player_suspend(Uint8 player);
void player_resume(Uint8 player);
/* what the item boxes give a player. a speed up's task doubles the
 * player's ground_accel, top_speed and still_grip, and puts them back as it
 * ends. a magnetic barrier or a barrier replaces the other, and
 * invincibility starts its music. a player of PLAYER_COUNT or more reads
 * player_work past its end, and becomes player 0 where that word is null.
 * otherwise its number stands, and each function and the speed up's task
 * read player_records past its end. a player whose slot is empty gets
 * nothing */
void player_give_speed_up(Uint8 player);
void player_give_magnetic_barrier(Uint8 player);
void player_give_barrier(Uint8 player);
void player_give_invincibility(Uint8 player);
/* set what the player looks at: task, offset by offset, or pos or ang. the
 * look keeps task until it's cleared or replaced, so task must stay alive.
 * offset, pos and ang are copied, and a null one leaves the vector or the
 * angles as they were. an empty slot is left alone */
void player_set_look_task(Uint8 player, struct task *task, NJS_POINT3 *offset);
void player_set_look_point(Uint8 player, NJS_POINT3 *pos);
void player_set_look_angles(Uint8 player, struct angles *ang);
void player_clear_look(Uint8 player);
/* look at task from a zero offset, or at pos or ang, as the
 * player_set_look_ functions do. a timer task made for the look clears it
 * on the timer's (steps + 2)th step. an empty slot, a null task, pos or ang,
 * or a timer task that can't be made does nothing. an older timer can clear a
 * newer look, so timed looks must not overlap */
void player_look_at_task(Uint8 player, struct task *task, int steps);
void player_look_at_point(Uint8 player, NJS_POINT3 *pos, Uint32 steps);
void player_look_at_angles(Uint8 player, struct angles *ang, Uint32 steps);
/* knock the player back, in PLAYER_MODE_KNOCK_BACK, at speed x, y, z */
void player_knock_back(Uint8 player, Float x, Float y, Float z);
/* knock players one and other apart, each by player_knock_back. their
 * speed is speed along the line between them, with its y then dropped, so
 * players at different heights part slower than speed */
void player_knock_apart(Uint8 one, Uint8 other, Float speed);
/* the submodes player_set_mode asks for: the clear's pose, standing again
 * as an act starts or an event lets the player go, and down */
#define PLAYER_MODE_CLEAR 19
#define PLAYER_MODE_RESET 24
#define PLAYER_MODE_DOWN 50
/* the submodes the game's objects and setters ask for */
#define PLAYER_MODE_PATH 1 /* along a path, faster */
#define PLAYER_MODE_SPRING 3 /* a spring's bounce */
#define PLAYER_MODE_LAUNCH 4 /* a launch along angles */
#define PLAYER_MODE_FLING 5 /* a fling along a line */
#define PLAYER_MODE_STILL 12 /* held still while an object moves it */
/* holding on to a rocket as it takes off, then flying with it, along or up */
#define PLAYER_MODE_ROCKET 20
#define PLAYER_MODE_ROCKET_ALONG 21
#define PLAYER_MODE_ROCKET_UP 22
#define PLAYER_MODE_JOIN_PATH 26 /* onto a path, where it's nearest */
/* hanging from a path and moving along it the way it runs */
#define PLAYER_MODE_HANG 29
/* hanging from a ledge and moving along it as the stick points, seen from
 * the camera */
#define PLAYER_MODE_LEDGE 31
#define PLAYER_MODE_CAUGHT 33 /* caught in the robo's grabber */
#define PLAYER_MODE_BREATHE 36 /* a breath of air */
#define PLAYER_MODE_HOLD 37 /* holding a task it's handed */
/* the submodes the stages' objects ask for, starting with the three
 * bounces of a spring that sends the player higher each time. the third
 * sends it off as a spring does */
#define PLAYER_MODE_BOUNCE_1 6
#define PLAYER_MODE_BOUNCE_2 7
#define PLAYER_MODE_BOUNCE_3 8
/* held as a bouncing spring squashes under the player, before the second
 * bounce and the third. PLAYER_MODE_STILL holds it before the first */
#define PLAYER_MODE_SQUASH_2 9
#define PLAYER_MODE_SQUASH_3 10
#define PLAYER_MODE_UPDRAFT 11 /* carried up by the wind */
#define PLAYER_MODE_BALL 13 /* curled up and spinning where it is */
#define PLAYER_MODE_HANG_ON 16 /* below a carrier that takes it along */
/* drawn onto a ride, then riding it */
#define PLAYER_MODE_GET_ON 17
#define PLAYER_MODE_RIDE 18
#define PLAYER_MODE_KNOCK_BACK 35 /* from player_knock_back */
#define PLAYER_MODE_FREEZE 38
#define PLAYER_MODE_DROWN 39 /* as its air runs out */
/* other submodes */
#define PLAYER_MODE_PATH_SPIN 2 /* along a path, curled up */
/* held still as PLAYER_MODE_STILL holds it, but running, or pushing */
#define PLAYER_MODE_STILL_RUN 14
#define PLAYER_MODE_STILL_PUSH 15
#define PLAYER_MODE_LAUNCH_SPEED 25 /* sent off by player_launch_speed */
/* off the ride PLAYER_MODE_RIDE holds it on, as a spring sends it */
#define PLAYER_MODE_RIDE_OFF 28
/* hanging below record->held, or holding on beside it to go round it */
#define PLAYER_MODE_DANGLE 30
#define PLAYER_MODE_CIRCLE 32
#define PLAYER_MODE_READY 34 /* a pose that races and rivals ask for */
#define PLAYER_MODE_WALL_FALL 43 /* down a wall */
#define PLAYER_MODE_BOARD 44 /* onto a board */
#define PLAYER_MODE_SLOPE_ROLL 45 /* curled up, as gravity alone rolls it */
#define PLAYER_MODE_SUPER 46
#define PLAYER_MODE_SUPER_END 47
/* the submodes npc.c and the dash rings ask of Tails */
#define PLAYER_MODE_FLY 48
#define PLAYER_MODE_HOVER 49 /* out of sight */
/* the submodes two stages' own tasks ask for. Lost World's task slides the
 * player down a path, posed in one of its actions, as the stick steers it
 * across */
#define PLAYER_MODE_PATH_SLIDE 23
/* under Casinopolis' shower, its object O SHWR, and let go of it */
#define PLAYER_MODE_SHOWER 40
#define PLAYER_MODE_SHOWER_END 41
/* in an action for Casinopolis' object O MUDOUT, which fades the mud */
#define PLAYER_MODE_MUD_OUT 42
/* ask the player for submode mode, through work->submode and
 * PLAYER_REQUEST. work->mode stays the character's own until its step takes
 * up the request, and player_finish_step clears PLAYER_REQUEST */
void player_set_mode(Uint8 player, Uint8 mode);
/* put the player on a path: in PLAYER_MODE_JOIN_PATH where it's nearest,
 * and at a point of it, in PLAYER_MODE_HANG, or in PLAYER_MODE_LEDGE turned
 * to ang. the player keeps path while it's on it, and point must index it.
 * only player_join_path leaves an empty slot alone */
void player_join_path(Uint8 player, struct path *path);
void player_hang_path(Uint8 player, struct path *path, int point);
void player_ledge_path(Uint8 player, struct path *path, int point,
                       struct angles *ang);
/* run the player along a path, in PLAYER_MODE_PATH with 5 added to its
 * speed, or curled up, in PLAYER_MODE_PATH_SPIN. the player keeps path */
void player_run_path(Uint8 player, struct path *path);
void player_spin_path(Uint8 player, struct path *path);
/* which of players 0 and 1 run along the path: 1 or 2, 3 for both, or 0 */
int player_on_path(struct path *path);
/* the player goes round the task, in PLAYER_MODE_CIRCLE, or hangs below
 * it, in PLAYER_MODE_DANGLE, unless it's holding something */
void player_circle(Uint8 player, struct task *task);
void player_dangle(Uint8 player, struct task *task);
/* bounce the player off a spring, in PLAYER_MODE_SPRING, at speed x, y, z */
void player_bounce(Uint8 player, Float x, Float y, Float z);
/* bounce the player off a spring that sends it higher each time, at the
 * first bounce, the second or the third */
void player_bounce_higher(Uint8 player, Uint8 bounce, Float x, Float y,
                          Float z);
/* hold the player as the spring squashes under it, before each bounce */
void player_squash(Uint8 player, Uint8 bounce);
/* launch the player, in PLAYER_MODE_LAUNCH, up through angles x, y and z
 * at speed, beyond its control for steps */
void player_launch(Uint8 player, const Angle *ang, Float speed, Sint16 steps);
/* launch the player in PLAYER_MODE_LAUNCH_SPEED at a speed of its own, in
 * the frame the angles turn to, beyond its control for steps */
void player_launch_speed(Uint8 player, NJS_VECTOR *speed, const Angle *ang,
                         Sint16 steps);
/* fling the player at speed along line, in PLAYER_MODE_FLING */
void player_fling(Uint8 player, Float speed, NJS_VECTOR *line);
/* turn the player to ang and send it off at speed, in the frame it then
 * faces, its stick ignored for steps */
void player_send_flying(Uint8 player, NJS_VECTOR *speed, const Angle *ang,
                        Sint16 steps);
/* turn the player to angles x and z, facing and aiming 0x4000 less y */
void player_set_angles(Uint8 player, Angle x, Angle y, Angle z);
/* turn the player to face along x, y, z */
void player_face_along(Uint8 player, Float x, Float y, Float z);
/* put the player at x, y, z, in its history too */
void player_set_position(Uint8 player, Float x, Float y, Float z);
/* TRUE while the player is on the ground, with *speed set to its speed
 * ahead, else FALSE */
Bool player_ground_speed(Uint8 player, Float *speed);
/* set the player's motion->speed to x, y, z in the world. record->speed
 * takes the same speed in the frame the player faces, with an x or z within
 * 0.001 of 0 zeroed */
void player_set_speed(Uint8 player, Float x, Float y, Float z);
/* set what pushes the player this step, its motion's accel */
void player_set_accel(Sint8 player, Float x, Float y, Float z);
/* set the player's push as the wind carries it up, in PLAYER_MODE_UPDRAFT */
void player_updraft(Sint8 player, Float x, Float y, Float z);
/* set the player's push, divided by its character's gravity */
void player_push(Sint8 player, Float x, Float y, Float z);
/* send the player off the ride, in PLAYER_MODE_RIDE_OFF, at speed x, y, z */
void player_ride_off(Uint8 player, Float x, Float y, Float z);
/* write pos and ang into the player's history, steps back from the latest
 * entry and wrapping round it, skipping either that is null. FALSE for an
 * empty slot, else TRUE */
Bool player_set_history(Uint8 player, int steps, NJS_POINT3 *pos,
                        struct angles *ang);
/* read the player's history into pos and ang, the same way */
Bool player_get_history(Uint8 player, int steps, NJS_POINT3 *pos,
                        struct angles *ang);

#endif

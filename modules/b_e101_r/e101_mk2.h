/* E-101 mkII, the boss E-102 fights in B_E101_R: its modes, its record and
 * its task */
#ifndef B_E101_R_E101_MK2_H
#define B_E101_R_E101_MK2_H
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/motion.h"
#include "system/task.h"

/* the boss's modes, in its work's mode */
enum e101_mk2_mode {
    E101_MK2_INTRO, /* hover still while the camera swings round it */
    E101_MK2_HOVER, /* circle near player 0 until its next attack */
    E101_MK2_LEAP, /* leap past the floor's middle, away from player 0 */
    E101_MK2_WARP, /* blur, and reappear high on the camera's side */
    E101_MK2_SWAT, /* swing an arm at E-102's shot, and knock it away */
    E101_MK2_MISSILES,
    E101_MK2_DASH, /* dash at where player 0 will be */
    E101_MK2_BEAM, /* fire the beam once */
    E101_MK2_BEAMS, /* fire the beam seven times */
    E101_MK2_HURT,
    E101_MK2_DEFEAT,
    E101_MK2_PAUSE /* wait until A or B is pressed. no code asks for it */
};

/* the bits of struct e101_mk2's flags */
#define E101_MK2_SWAYS 0x1 /* its upper body leans out of a turn */
#define E101_MK2_GUARDED 0x2 /* a hit does it no harm */
/* its model isn't drawn, and hover_y stays. no code sets it */
#define E101_MK2_HIDDEN 0x4
#define E101_MK2_STILL 0x8 /* its y angle doesn't turn */
/* it holds its height, but never below hover_y */
#define E101_MK2_FLOATS 0x10
#define E101_MK2_BLUR 0x20 /* copies of it trail behind it */
/* copies of it, each a little bigger, take its pose now, as it warps */
#define E101_MK2_WARPING 0x40
#define E101_MK2_TRAIL_ARM0 0x100 /* arm 0 trails light */
#define E101_MK2_TRAIL_ARM1 0x200 /* arm 1 trails light */
#define E101_MK2_TILTS 0x400 /* it tilts with its speed, and wobbles */
#define E101_MK2_FROZEN 0x800 /* its motion doesn't step */
#define E101_MK2_LEADS 0x1000 /* it turns to face aim, not player 0 */
/* it leapt from a shot, and hovers before its next attack */
#define E101_MK2_DODGED 0x2000
#define E101_MK2_THRUST 0x4000 /* its back puffs thrust, as it dashes */

/* the boss's health at the start. each hit takes 1 */
#define E101_MK2_HEALTH 4

/* E-101 mkII's record, its task's extra. motion_create allocates it, so its
 * motion comes first */
struct e101_mk2 {
    struct motion motion;
    Float health;
    Sint32 flags;
    Sint32 steps; /* the steps since its intro began */
    /* the steps toward its next attack. a hover starts the count at 0. a
     * hover after a swat or a leap goes on counting from where it was */
    Sint32 hover_steps;
    Sint32 timer; /* the steps a mode counts, up or down */
    /* the mode it takes at the end of the step, and the mode it left */
    Sint32 next_mode;
    Sint32 last_mode;
    /* how many copies of it trail it, and of each arm's light, from 0 to 4 */
    Sint32 blur_copies;
    Sint32 trail_copies[2];
    /* the tilt and the wobble it is drawn with, on top of its work's
     * angles */
    Angle pitch;
    Angle roll;
    /* the y angle it turns toward: its heading seen from player 0, or with
     * E101_MK2_LEADS, seen from aim. it faces back along its y angle */
    Angle aim_y;
    Angle turn_speed; /* its y angle's turn each step, in turn_smoothly */
    Angle orbit; /* the heading of target seen from player 0 */
    Angle orbit_turn; /* orbit's turn each step */
    Angle orbit_aim; /* the heading of the floor's middle seen from player 0 */
    Float player_speed; /* player 0's speed in x and z */
    /* its speed along its y angle in a dash. it is negative, as the dash runs
     * the way the boss faces */
    Float move_speed;
    /* the square of its distance from target in x and z */
    Float target_distance2;
    /* hover_y as the draw found it, before setting it anew. while this reads
     * MIN_HOVER_Y, the draw copies the new hover_y here instead. nothing but
     * the draw reads it */
    Float last_hover_y;
    Float hover_y; /* the height it falls or rises to */
    Float height; /* its height above hover_y, which nothing reads */
    /* how many steps of player 0's velocity aim lies ahead of player 0 */
    Float lead;
    NJS_POINT3 last_pos; /* its position before move last moved it */
    NJS_VECTOR velocity; /* the distance it moves each step */
    NJS_POINT3 target; /* the point it steers toward as it hovers */
    NJS_POINT3 destination; /* where it leaps or warps to */
    /* where player 0 will be: player 0's position plus lead steps of its
     * velocity in x and z */
    NJS_POINT3 aim;
    NJS_POINT3 cannon; /* the point between its hands where the beam charges */
    Uint8 unk_e0[4];
    /* where each jet's flames and its back's thrust start, and the way each
     * goes. the jets' task keeps pointers to jets and jet_dirs, not copies */
    NJS_POINT3 jets[2];
    NJS_VECTOR jet_dirs[2];
    NJS_POINT3 thrust_pos;
    NJS_VECTOR thrust_dir;
    Sint32 trail_color; /* the index of the colors its arms' light takes */
    /* the size of its wobble in degrees, and the wobble's phases and their
     * turns each step. x's phase pitches it, and z's rolls it */
    Float wobble;
    struct angles wobble_phase;
    struct angles wobble_step;
    /* bob is how far above its position it is drawn. bob_speed is bob's
     * change each step, and bob_damping the share of it lost each step */
    Float bob;
    Float bob_speed;
    Float bob_damping;
    Sint32 attack_index; /* the index of its next attack in its row */
    /* health as a whole number, which picks its rows of attack_rows and
     * missile_rows */
    Sint32 health_units;
    Sint32 beams_left; /* shots of the beam still to fire */
    /* the chance that a hover's next attack is missiles, and how many it
     * fires */
    Float missile_chance;
    Sint32 missiles;
    /* 30 for each hit it has taken. the beam's last shot holds that many
     * steps. its blast's size is 40 plus damage */
    Float damage;
    struct task *beam;
    Float sway; /* SWAY_OBJECT's move along its z as it leans out of a turn */
};

/* make E-101 mkII's task at level 2, unless no task is free. it starts with
 * its intro, and its defeat clears the stage. E101R.NB and E101R.PB must be
 * loaded, as e101_r_load loads them, and player 0's task must exist.
 *
 * the allocations of its lock-on record, its motion record and its shapes'
 * copy go unchecked. without the lock-on record, enemy_alloc_record kills
 * the task. but e101_mk2_create then sets its step again, so the boss runs
 * on. as it hovers, it then reads the lock-on record through null whenever
 * it looks for a shot. so does a hit that lands while it is unguarded.
 * without the motion record, e101_mk2_create writes through null. without
 * the shapes' copy, the boss's first step reads through null */
void e101_mk2_create(void);

#endif

/* the camera: its modes, the act's collisions that pick them, and overrides */
#ifndef CAMERA_CAMERA_H
#define CAMERA_CAMERA_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

struct path;

/* how camera_work is placed from camera_state after the mode's step */
enum camera_type {
    CAMERA_BY_STEP, /* the step places camera_work itself */
    CAMERA_FROM_TARGET, /* distance back from target along the angles */
    CAMERA_LOOK_AT, /* at pos, looking at target */
    CAMERA_AT_ANGLES /* at pos, turned by the angles */
};

/* camera_start_path's modes, which say when its step ends with
 * camera_restore. a mode that holds the path's end keeps the camera at the
 * path's last point once the path runs out. a mode not named here ends the
 * step at once */
#define CAMERA_PATH_ONCE 0 /* at the path's end */
#define CAMERA_PATH_KEEP 1 /* never, holding the end */
#define CAMERA_PATH_KEEP_2 2 /* as CAMERA_PATH_KEEP */
/* once the mode has run steps steps, holding the end till then */
#define CAMERA_PATH_HOLD 3
#define CAMERA_PATH_HOLD_2 4 /* as CAMERA_PATH_HOLD */

/* the steps a character's clear pose runs its camera path for, under
 * CAMERA_PATH_HOLD */
#define CLEAR_PATH_STEPS 720

/* the view a mode's step reads. below DEBUG, it is the mode's collision's,
 * or camera_override_view under CAMERA_VIEW_GIVEN. at DEBUG,
 * camera_step_editor sets it. positions are in the world, and an angle takes
 * 65536 units to a turn. some steps read members as values of their own.
 * TORNADE reads pos.y as a height above player 0. SONIC_P reads target and
 * distance as four distances */
struct camera_view {
    /* some steps read these as unsigned values of their own. POINT sweeps
     * the camera's move for walls only while ang_x is 0. KLAMATH takes
     * ang_x as its height in tenths, or half the distance while ang_x is 0.
     * it rises while ang_y is 0 and player 0 is off the ground. SONIC_P
     * takes ang_x as its pitch, and drifts while ang_y isn't 0 */
    Sint16 ang_x;
    Sint16 ang_y;
    NJS_POINT3 pos;
    NJS_POINT3 target;
    Float distance;
    Sint32 steps; /* the mode's steps so far: 0 in its first */
};

/* in camera_state's flags: L or R turned the camera. the next step of
 * camera_adjust_normal or camera_adjust_slow clears the flag, and leaves the
 * camera's y angle as the mode set it */
#define CAMERA_TURNED 0x1

/* where the mode's step leaves the camera, from which a camera_type places
 * camera_work */
struct camera_state {
    Uint8 view; /* which view the mode shows, by its own numbering */
    Uint8 phase; /* where the mode is, by its own numbering */
    Uint8 unk_02[4];
    Sint16 flags;
    Float distance;
    NJS_POINT3 pos;
    NJS_POINT3 target;
    Angle ang_x, ang_y, ang_z;
    Uint8 unk_30[12];
    Angle spin_x, spin_y, spin_z; /* the boss camera's turn each step */
    NJS_VECTOR move; /* the boss camera's move this step */
};

/* the levels a mode runs at, lowest first, as the camera editor names
 * them. the camera tests a collision only while the collision's mode runs
 * at the camera's level or higher. at DEBUG, camera_step_editor sets the
 * view before each step */
enum camera_level {
    CAMERA_LEVEL_NORMAL,
    CAMERA_LEVEL_AREA,
    CAMERA_LEVEL_EVENT,
    CAMERA_LEVEL_COMP,
    CAMERA_LEVEL_COLI,
    CAMERA_LEVEL_DEBUG,
    CAMERA_LEVELS
};

/* one of the camera's modes, by number */
struct camera_mode {
    void (*step)(struct camera_view *view);
    Sint8 level; /* the camera_level the mode runs at */
    Sint8 type; /* a camera_type */
    Uint8 unk_06[2];
    Bool pickable; /* TRUE for a mode the pad's camera buttons can pick */
    /* the step the camera editor gives camera_take as it edits a
     * collision's camera points or distance, or NULL for camera_orbit */
    void (*editor)(struct camera_view *view);
};

/* the modes by number, as the camera editor names them. a mode's A_, C_
 * and COL_ copies run at CAMERA_LEVEL_AREA, COMP and COLI */
#define CAMERA_MODE_FOLLOW 0
/* the mode the adventure fields start the camera in */
#define CAMERA_MODE_KNUCKLES 4
#define CAMERA_MODE_KNUCKLES2 8
#define CAMERA_MODE_COL_KNUCKLES2 11
#define CAMERA_MODE_ASHLAND 20
#define CAMERA_MODE_C_ASHLAND 22
#define CAMERA_MODE_ASHLAND_I 23
#define CAMERA_MODE_A_ASHLAND_I 24
#define CAMERA_MODE_C_ASHLAND_I 25
#define CAMERA_MODE_FIXED 28
#define CAMERA_MODE_A_FIXED 29
#define CAMERA_MODE_C_FIXED 30
#define CAMERA_MODE_ADVERTISE 45 /* the menus' camera */
/* the ridden cart's camera, unless camera_cart_action gives its step
 * another job */
#define CAMERA_MODE_CART 49
#define CAMERA_MODE_CHAOS 50
/* Chaos 0's camera while he is more than 60 above player 0 */
#define CAMERA_MODE_CHAOS_P 51
/* the mode camera_start_swing's swing runs in */
#define CAMERA_MODE_CHAOS_STINIT 52
#define CAMERA_MODE_CHAOS_STD 53
#define CAMERA_MODE_CHAOS_W 54 /* B_CHAOS7.PRS's chaos7_camera_entry */
#define CAMERA_MODE_E101R 55
#define CAMERA_MODE_E103 56
#define CAMERA_MODE_EGM3 57 /* B_EGM3.PRS's eggmobile3_camera_step */
/* a plane collision of this mode, crossed at EVENT, sets only the adjust
 * mode the camera goes back to at NORMAL */
#define CAMERA_MODE_COLLI 61
/* at camera_work's scl, looking at the target its vars hold */
#define CAMERA_MODE_RUIN_WAKA1 62
#define CAMERA_MODE_SURVEY 64 /* the pad's look round */
/* the camera sits 2 out from camera_place's pos along its ang. it turns by
 * that ang, by a half turn, and by pad 0's stick */
#define CAMERA_MODE_TAIHO 65
#define CAMERA_MODE_LEAVE 68 /* stays where it is, looking at player 0 */
#define CAMERA_MODES 72

/* the adjust modes by number, as the camera editor names them */
#define CAMERA_ADJUST_NONE 0
#define CAMERA_ADJUST_NORMAL 1
#define CAMERA_ADJUST_NORMAL_S 2
#define CAMERA_ADJUST_SLOW 3
#define CAMERA_ADJUST_SLOW_S 4
#define CAMERA_ADJUST_TIME 5
#define CAMERA_ADJUST_THREE1 6
#define CAMERA_ADJUST_THREE1C 7
#define CAMERA_ADJUST_THREE2 8
#define CAMERA_ADJUST_THREE2C 9
#define CAMERA_ADJUST_THREE3 10
#define CAMERA_ADJUST_THREE3C 11
#define CAMERA_ADJUST_THREE4 12
#define CAMERA_ADJUST_THREE4C 13
#define CAMERA_ADJUST_THREE5 14
#define CAMERA_ADJUST_THREE5C 15
#define CAMERA_ADJUST_RELATIVE1 16
#define CAMERA_ADJUST_RELATIVE1C 17
#define CAMERA_ADJUST_RELATIVE2 18
#define CAMERA_ADJUST_RELATIVE2C 19
#define CAMERA_ADJUST_RELATIVE3 20
#define CAMERA_ADJUST_RELATIVE3C 21
#define CAMERA_ADJUST_RELATIVE4 22
#define CAMERA_ADJUST_RELATIVE4C 23
#define CAMERA_ADJUST_RELATIVE5 24
#define CAMERA_ADJUST_RELATIVE5C 25
#define CAMERA_ADJUST_RELATIVE6C 26
#define CAMERA_ADJUSTS 27

/* the adjust mode's own record, which its step keeps */
struct camera_adjust_work {
    Sint16 flags;
    Uint8 unk_02[2];
    /* the turn the NORMAL and SLOW modes last gave each angle */
    Angle turn_x;
    Angle turn_y;
    Uint8 unk_0c[4];
    Float move; /* the move the SLOW modes last gave the camera */
    Sint32 steps; /* the adjust mode's steps so far: 0 in its first */
};

/* one of the adjust modes, by number. its step runs after the mode's, with
 * the camera's work as the mode left it and as it was before */
struct camera_adjust_mode {
    /* nonzero for an adjust mode that waits out its mode's first step */
    Sint32 waits;
    void (*step)(struct task_work *work, struct task_work *last,
                 struct camera_adjust_work *adjust);
};

/* in camera_control's flags: the adjust mode waits out the mode's first
 * step */
#define CAMERA_ADJUST_WAITS 0x1
/* below DEBUG, the step reads camera_override_view, not its collision's
 * view */
#define CAMERA_VIEW_GIVEN 0x2

/* the mode the camera runs, and its adjust mode. each level has a slot
 * that keeps what ran before the camera entered the level. the camera goes
 * back to it as the level's mode ends */
struct camera_control {
    /* set by a change of mode, and cleared as the camera task restarts
     * steps */
    Bool changed;
    Sint16 collision; /* the collision whose mode it is, or -1 */
    Sint8 mode; /* the mode, or -1 for a step no mode names */
    Sint8 type; /* a camera_type */
    Sint8 level; /* a camera_level */
    Sint8 adjust;
    Uint8 unk_0a;
    Sint8 flags;
    void (*step)(struct camera_view *view);
    void (*adjust_step)(struct task_work *work, struct task_work *last,
                        struct camera_adjust_work *adjust);
    Sint32 steps; /* the mode's steps so far: 0 in its first */
    /* the slots, by level */
    Sint32 saved_level[CAMERA_LEVELS];
    Sint16 saved_collision[CAMERA_LEVELS];
    Sint8 saved_flags[CAMERA_LEVELS];
    void (*saved_step[CAMERA_LEVELS])(struct camera_view *view);
    void (*saved_adjust_step[CAMERA_LEVELS])(struct task_work *work,
                                             struct task_work *last,
                                             struct camera_adjust_work *adjust);
    Sint8 saved_mode[CAMERA_LEVELS];
    Sint8 saved_adjust[CAMERA_LEVELS];
    Sint8 saved_type[CAMERA_LEVELS];
    /* camera_take copies the level into DEBUG's entry, as into
     * saved_level's. nothing reads it */
    Sint16 unk_86[CAMERA_LEVELS];
    /* the offsets camera_add_offset gave since the last view from the work,
     * added up */
    NJS_VECTOR offset;
};

/* a camera collision's shape */
enum camcol { CAMCOL_SPHERE, CAMCOL_PLANE, CAMCOL_BLOCK, CAMCOL_TYPES };

/* one of an act's camera collisions: a shape, and the mode and view it
 * gives the camera. player 0 inside its sphere or block, or crossing its
 * plane at NORMAL, can give the camera its mode. the camera's level, the
 * mode's level and the collisions' priorities decide whether it does. a
 * collision whose mode runs at COLI gives no mode. camera_push_out pushes
 * points out of its sphere or block instead. at EVENT, crossing its plane
 * sets the mode and adjust mode the camera goes back to at NORMAL */
struct camera_collision {
    Sint8 mode;
    Sint8 priority;
    Uint8 adjust;
    Sint8 type; /* a camcol shape */
    Uint16 col_ang_x;
    Uint16 col_ang_y;
    NJS_POINT3 col_pos;
    /* a sphere's radius in x and its square in z, or a plane's or block's
     * half size along each of its axes */
    NJS_POINT3 col_scl;
    Uint16 cam_ang_x;
    Uint16 cam_ang_y;
    NJS_POINT3 dir_pos; /* where the camera looks */
    NJS_POINT3 cam_pos;
    Float distance;
};

/* the most collisions the camera editor lets a layout hold */
#define CAMERA_COLLISIONS_MAX 0x1ff

/* a count of collisions, and the collisions */
struct camera_layout {
    Sint32 count;
    struct camera_collision *collisions;
};

/* in a camera_path's flags: walls between the path and the point the
 * camera follows pull the camera in toward the path */
#define CAMERA_STRETCH_WALLS 0x1
/* camera_step_path keeps the camera's last move, and moves it toward each
 * new move. a shipped bug has the kept move take each new move whole. the
 * camera then goes where it would without the flag, give or take rounding */
#define CAMERA_STRETCH_KEEP_MOVE 0x2
/* where a wall pulls the camera in, camera_step_path also casts from the
 * path's nearest point toward the point the camera follows. a wall there
 * too sets CAMERA_PATH_WALLED */
#define CAMERA_STRETCH_SIGHT 0x4
/* camera_step_path sets CAMERA_PATH_FAR while the point the camera follows
 * is more than the stretch's far from the path, and clears it otherwise */
#define CAMERA_STRETCH_MEASURE 0x8
/* the camera moves the stretch's ease of the way to its place each step */
#define CAMERA_STRETCH_EASE 0x10

/* a stretch of a path, and how camera_step_path follows the path. the
 * path's task tests a player against the stretch's points to hand the camera
 * to camera_step_path and to take it back, but camera_step_path follows the
 * whole path. the camera follows a point height above player 0. it takes the
 * point of the path sit_back behind that point, and the one look_ahead ahead
 * of it. it sits at the first and looks at the second, each moved out toward
 * the point it follows */
struct camera_path {
    Sint32 flags;
    struct path *path; /* set by the path's task as it hands over the camera */
    Float look_ahead;
    Float sit_back;
    /* where the camera looks, and where it sits, as shares of the way out
     * from the path to the point it follows */
    Float look_share;
    Float sit_share;
    Float unk_18;
    Float unk_1c;
    Float height; /* the point the camera follows is this far above player 0 */
    Float short_by; /* the way out from the path, cut short by this much */
    Float unk_28;
    Float ease; /* the share of the way to its place the camera moves a step */
    Float far; /* how far from the path CAMERA_PATH_FAR starts */
    Float unk_34;
    Sint32 first; /* the stretch's first point */
    Sint32 skip; /* the stretch stops this many points short of the end */
    /* camera_step_path compares the view's steps with it and ignores the
     * result */
    Uint32 wait;
    NJS_VECTOR shift; /* where the camera sits, moved by this */
    NJS_VECTOR path_shift; /* the path's points, moved by this */
    Uint8 unk_5c[12];
    Angle unk_68;
    Float turn; /* the share of the way to its aim the camera turns a step */
};

/* what camera_work's vars hold for the step that reads them:
 * camera_step_work_view looks at target from the work's scl, and
 * camera_step_path follows path */
union camera_vars {
    NJS_POINT3 target;
    struct camera_path *path;
};

#define CAMERA_VARS(work) ((union camera_vars *)&(work)->vars)

/* bits camera_step_path sets in camera_work's submode, telling the path
 * task the camera may be done with the stretch */
#define CAMERA_PATH_FAR 0x1 /* the point it follows is far from the path */
#define CAMERA_PATH_WALLED 0x2 /* walls cut off both ways out to that point */
/* the point the camera sits at or the one it looks at reached an end of
 * the path */
#define CAMERA_PATH_AT_END 0x4
#define CAMERA_PATH_DONE                                                       \
    (CAMERA_PATH_FAR | CAMERA_PATH_WALLED | CAMERA_PATH_AT_END)

/* a key of a camera path: where the camera is, and what it looks at */
struct camera_key {
    NJS_POINT3 pos;
    NJS_POINT3 target;
};

/* one of camera_place's two words for CAMERA_MODE_CART's step */
union camera_place_arg {
    struct camera_key *keys;
    Sint32 last;
    NJS_POINT3 *point;
};

/* a place for the camera's steps to read. the fishing game, the ridden cart
 * and the race fill it, as do the cart's intro and debug pages and the
 * ruin's mirrors */
struct camera_place {
    /* the fishing game's lure, a mirror's eye, the ridden cart, or the race
     * camera's place */
    NJS_POINT3 pos;
    /* the point the fishing game's camera or the race's looks at. the ridden
     * cart sets x to 16 while it is in the air, and to 0 otherwise.
     * CAMERA_MODE_CART's view 1 rises while x isn't 0 */
    NJS_POINT3 target;
    /* the way a mirror first turns the camera, or the ridden cart's or the
     * race camera's angles */
    struct angles ang;
    /* at CAMERA_CART_PLAY, the keys CAMERA_MODE_CART's step plays and the
     * index of the last. at CAMERA_CART_ORBIT, the point the step stores the
     * camera's position in, and the point the camera looks at */
    union camera_place_arg args[2];
    Float distance; /* the race camera's distance from target */
};

/* SHC lays out zeroed globals in the order of their first declarations, so
 * camera.c's are all declared here, in their order in memory, even those
 * only camera.c reads */

extern struct camera_place camera_place;
extern Uint8 dat_8c6c0a30[4];
/* TRUE while player 0 is on land, and moved less than 0.1 since the
 * camera's last step */
extern Bool camera_player_still;
/* how far the camera moved last step */
extern NJS_VECTOR camera_moved;
/* what camera.c's camera_last and camera_state point at */
extern struct task_work camera_last_record;
extern struct camera_state camera_state_record;
extern Uint8 dat_8c6c0ad8[12];
/* what camera.c's camera_control points at */
extern struct camera_control camera_control_record;
/* the view the mode's step is handed */
extern struct camera_view camera_step_view;
/* the view copied into camera_step_view under CAMERA_VIEW_GIVEN, below
 * DEBUG */
extern struct camera_view camera_override_view;
/* what camera.c's camera_adjust points at */
extern struct camera_adjust_work camera_adjust_record;
/* the stage and act whose collisions the camera reads */
extern Sint16 camera_stage;
extern Sint16 camera_act;
/* how many modes there are and the modes by number, then the same for the
 * adjust modes. camera_create and camera_create_advertise set them. no call
 * checks a number it takes against its count */
extern Sint16 camera_mode_count;
extern struct camera_mode *camera_modes;
extern Sint16 camera_adjust_count;
extern struct camera_adjust_mode *camera_adjusts;
/* the count of the act's collisions, and the collisions. the first sets
 * the act's default mode, and the camera tests only the rest */
extern Sint32 *camera_collision_count;
extern struct camera_collision *camera_collisions;
/* the field of view the camera keeps */
extern Angle camera_perspective;
/* an action_state, as camera.c names them: no camera action, one that
 * camera_play_action gave since the camera's last step, or one that step
 * played */
extern Sint32 camera_action_state;
/* the action camera_play_action gave, and the frame it plays at */
extern NJS_CACTION *camera_action;
extern Float camera_action_frame;
/* camera task steps since the camera started at player 0,
 * camera_change_act or camera_reset_steps. the adjust modes wait till it
 * passes 60 */
extern Uint32 camera_steps;
/* steps since camera_restore, camera_change_act or the camera's start at
 * player 0. while collisions are on, each step counts twice. the pad starts
 * the survey only once it passes 10 */
extern Uint32 camera_restored_steps;
/* the mode camera_set_start_mode gave, and the mode and adjust mode
 * camera_set_restore_mode gave. a mode of -1 is none */
extern Sint32 camera_start_mode;
extern Sint32 camera_restore_mode;
extern Sint32 camera_restore_adjust;
/* set as the camera moves to a collision of higher priority, and cleared
 * each step. such a move with adjust mode NONE cuts the view, and player 0
 * then loses control for 30 steps */
extern Bool camera_cut;
/* the camera task's steps after it starts the camera at player 0, counted
 * over every camera made. nothing reads it */
extern Sint32 camera_step_count;
/* the work camera_take sets for its step */
extern struct task_work *camera_take_work;
extern Uint8 dat_8c6c0c30[4];
/* where player 0 and the camera were last step */
extern NJS_POINT3 camera_player_last;
extern NJS_POINT3 camera_last_pos;
/* set each step the camera is in a box free of water, an
 * invisible_dry_step's. the tint under water skips a step that finds the
 * flag set, and clears it */
extern Bool camera_in_dry_box;

/* modes.c's zeroed globals up to camera_pose_ang follow, declared here in
 * their order in memory, so that SHC lays them out in that order */

/* LR's orbit of player 0: its distance across, its height above him and
 * its heading round him. the mode takes them from the camera as it
 * begins */
extern Float trigger_orbit_distance;
extern Float trigger_orbit_height;
extern Angle trigger_orbit_angle;
/* 1 while POINT's trigger orbit runs */
extern int trigger_orbit_active;
/* 4 bytes nothing reads */
extern Uint8 dat_8c6c0ce0[4];
/* 1 while POINT hands the camera to camera_step_follow */
extern int camera_point_follows;
/* the place camera_boss_place_from_player sets, which camera_boss_move moves
 * the boss camera toward */
extern NJS_POINT3 camera_boss_place;
/* the offsets camera_set_boss_offsets sets */
extern NJS_POINT3 camera_boss_target_offset;
extern NJS_POINT3 camera_boss_offset;
/* the limits camera_set_boss_limits sets */
extern struct angles camera_boss_turn_step;
extern struct angles camera_boss_spin_limit;
extern NJS_POINT3 camera_boss_move_limit;
/* camera_start_swing's swing: how far it has turned the camera round the
 * work, and how far it turns it a step */
extern Angle camera_swing_turned;
extern Angle camera_swing_turn;
/* the work the camera swings round */
extern struct task_work *camera_swing_work;
/* camera_start_swing's height, which nothing reads */
extern Float camera_swing_height;
/* the camera's distance from the swing's target, the distance it closes in
 * to, and how far it closes in a step */
extern Float camera_swing_distance;
extern Float camera_swing_near;
extern Float camera_swing_close_step;
/* 1 once camera_start_swing's swing has turned a quarter turn. the bosses
 * wait for it. camera_start_swing and chaos_setup clear it */
extern Sint8 camera_swing_done;
/* a swing_phase, as modes.c names them */
extern Sint8 camera_swing_phase;
/* the point the camera swings round, height over the work, and the
 * camera's heading */
extern NJS_POINT3 camera_swing_target;
extern Angle camera_swing_heading;
/* KLAMATH's height above player 0, and 1 while it hands the camera to
 * camera_step_follow */
extern Float camera_klamath_height;
extern int camera_klamath_follows;
/* what KLAMATH takes off the camera's height above player 0. it starts at
 * 0, and drops 0.2 each step player 0 is off the ground while the view's
 * ang_y is 0. each other step it eases a tenth of the way back to 0. it
 * never rises above 0, so taking it off raises the camera */
extern Float camera_klamath_sink;
/* 1 while ASHLAND hands the camera to camera_step_follow */
extern int camera_ashland_follows;
/* camera_step_pose sets camera_pose_taken as it gives the camera the pose
 * camera_pose_pos and camera_pose_ang hold */
extern Bool camera_pose_taken;
extern NJS_POINT3 camera_pose_pos;
extern struct angles camera_pose_ang;

/* camera_cart_action's values, which camera/cart.h declares. at
 * CAMERA_CART_PLAY, CAMERA_MODE_CART's step plays keys, and clears
 * camera_cart_action 70 steps after the last. at CAMERA_CART_ORBIT, pad 1
 * turns the camera about a point. camera_place's args say what each reads */
#define CAMERA_CART_PLAY 1
#define CAMERA_CART_ORBIT 2
/* the camera's work, from the camera task's first step until the task is
 * destroyed, and NULL otherwise */
extern struct task_work *camera_work;
/* where the camera's modes leave it */
extern struct camera_state *const camera_state;

/* a step that leaves the camera as it is: the COLLI mode's, and the one
 * camera_play_action gives camera_override while an action moves the
 * view */
void camera_step_none(struct camera_view *view);

/* set the mode the camera starts in, or -1 for the act's default.
 * camera_create sets it back to -1, so call this after it */
void camera_set_start_mode(Sint8 mode);
/* set the mode and adjust mode the camera takes when a restore returns it
 * to NORMAL, or mode -1 for none. camera_create sets the mode back to -1 */
void camera_set_restore_mode(Sint8 mode, Sint8 adjust);
/* the field of view, as njSetPerspective takes it, in 65536 units to a
 * turn. the camera starts it at 0x31c7, 70 degrees */
Angle camera_get_perspective(void);
/* set the field of view to ang and hand it to njSetPerspective. the three
 * FIXED modes ignore ang. they move the field of view toward 0x31c7 by at
 * most 18 a call, or with adjust mode NONE set 0x31c7 at once */
void camera_set_perspective(Angle ang);
/* whether the act's collisions pick the mode and the pad starts and ends
 * the survey, and the calls that turn that on and off. a new camera's first
 * step turns it on */
Bool camera_collisions_enabled(void);
void camera_enable_collisions(void);
void camera_disable_collisions(void);
/* make the camera for the stage and act stage_get_stage_act gives.
 * camera_change_act adds by to the act, and takes that act's collisions at
 * their default mode */
void camera_create(void);
void camera_change_act(Sint16 by);
/* make the menus' camera, on camera_advertise_layout's collisions, in
 * CAMERA_MODE_ADVERTISE */
void camera_create_advertise(void);
/* place the work as a camera_type says, from camera_state */
void camera_place_at_angles(struct task_work *work);
void camera_place_look_at(struct task_work *work);
void camera_place_from_target(struct task_work *work);
/* move the next view the camera applies from its work by offset. offsets
 * add up until that view, and a view from a camera action neither applies
 * nor clears them */
void camera_add_offset(NJS_VECTOR *offset);
/* at CAMERA_LEVEL_NORMAL, run mode with adjust mode adjust, and keep what
 * ran in NORMAL's slot. at any other level, do nothing */
void camera_set_mode(Sint16 mode, Uint8 adjust);
/* run mode with adjust mode adjust at CAMERA_LEVEL_EVENT, its step reading
 * camera_override_view. from below EVENT, EVENT's slot keeps what ran; at
 * EVENT, the slot keeps what it holds. above EVENT, do nothing */
void camera_override_mode(Sint16 mode, Uint8 adjust);
/* camera_override_mode for a step no mode names, after which type, a
 * camera_type, places camera_work. step, and what it reads, must stay
 * loaded while it runs or while a slot keeps it */
void camera_override(void (*step)(struct camera_view *view), Uint8 adjust,
                     Sint8 type);
/* at CAMERA_LEVEL_EVENT, go back to what EVENT's slot keeps. if that runs
 * at NORMAL, take the mode camera_set_restore_mode gave instead, if any. at
 * every level, zero camera_restored_steps */
void camera_restore(void);
/* camera_override and camera_restore at CAMERA_LEVEL_COLI, which only DEBUG
 * is above. camera_restore_high leaves camera_restored_steps as it is */
void camera_override_high(void (*step)(struct camera_view *view), Uint8 adjust,
                          Sint8 type);
void camera_restore_high(void);
/* run step at CAMERA_LEVEL_DEBUG, with camera_take_work set to work, which
 * must outlive the step's use of it. the step places camera_work itself.
 * from below DEBUG, DEBUG's slot keeps what ran. nothing goes back to it:
 * camera_release, at DEBUG, takes the act's default mode at NORMAL */
void camera_take(struct task_work *work,
                 void (*step)(struct camera_view *view));
void camera_release(void);
/* whether the camera runs at exactly CAMERA_LEVEL_EVENT, or exactly COMP */
Bool camera_at_event_level(void);
Bool camera_at_comp_level(void);
/* camera task steps since the camera started at player 0, camera_change_act
 * or camera_reset_steps */
Uint32 camera_get_steps(void);
void camera_reset_steps(void);
/* the adjust mode, which camera_set_adjust starts at step 0 */
void camera_set_adjust(int adjust);
int camera_get_adjust(void);
/* the mode the camera runs, or -1 while a step no mode names runs */
int camera_get_mode(void);
/* TRUE at TIME and at each adjust mode whose name ends in C: the adjust
 * modes whose step sweeps the camera's move for walls itself */
Bool camera_adjust_is_c(void);
/* set the camera_type that places camera_work after the mode's step */
void camera_set_type(int type);
/* show action at frame as the view in the camera's next step. action must
 * stay valid till that step. it returns FALSE when the camera takes the
 * action. it returns TRUE when a call since the camera's last step gave one
 * already, which the camera keeps. a call while no action plays first runs
 * camera_override with camera_step_none. a camera step with no call since
 * its last ends the action with camera_restore */
Bool camera_play_action(NJS_CACTION *action, Float frame);
/* push pos out of each of the act's spheres and blocks whose mode runs at
 * CAMERA_LEVEL_COLI. it returns how many held pos */
int camera_push_out(NJS_POINT3 *pos);
/* move pos toward from by n 30ths of the way, n a countdown of steps, and
 * count n down. n starts at 0 and nothing sets it, so pos stays as it is */
void camera_ease(NJS_POINT3 *from, NJS_POINT3 *pos);
/* world_sweep_ball_hit for the camera, which water lets through */
Bool camera_sweep_hits(NJS_POINT3 *pos, NJS_VECTOR *speed, struct angles *ang,
                       Float radius);

/* a step for camera_take: while pad 0 holds Start, the stick turns the
 * camera about camera_take_work, and L and R move it out and in by 2. it
 * keeps at least 5 from the work */
void camera_orbit(struct camera_view *view);
/* steps for camera_override: an event's, which runs event_camera_step, and
 * one that sets the camera to the view's pos and target */
void camera_step_event(struct camera_view *view);
void camera_step_fixed(struct camera_view *view);
/* FISHING's step, which Big also gives camera_override as he casts. the
 * camera looks at camera_place's target from 50 beyond a point 20 above
 * player 0 */
void camera_step_fishing_point(struct camera_view *view);
/* a step that looks at the view's target from 50 beyond a point 7.5 above
 * player 0. a rocket that flies up gives it to camera_override as player 0
 * rides it */
void camera_step_view_target(struct camera_view *view);
/* a step that keeps the camera where it is, looking at player 0's eyes */
void camera_step_eyes(struct camera_view *view);
/* the two steps a board trick picks between at random for camera_override.
 * each moves the camera round player 0 along a path of its own, turned the
 * way he faces. once the path ends or he lands, the step hands the camera
 * back to camera_step_board */
void camera_step_trick_a(struct camera_view *view);
void camera_step_trick_b(struct camera_view *view);
/* the step Ice Cap gives the camera for its board run, which follows player
 * 0 on the board */
void camera_step_board(struct camera_view *view);
/* the KNUCKLES and KNUCKLES2 modes' step, which follows player 0. Tails
 * also gives it to camera_override as he flies */
void camera_step_follow(struct camera_view *view);
/* steps for camera_override that camera_work drives: one puts the camera
 * at the work's scl, looking at the target in its vars, and one follows
 * the path of the stretch its vars hold */
void camera_step_work_view(struct camera_view *view);
void camera_step_path(struct camera_view *view);
/* a step for camera_override that places the camera at camera_pose_pos,
 * turned by camera_pose_ang, and sets camera_pose_taken. the stages' path
 * cameras set the pose each step */
void camera_step_pose(struct camera_view *view);
/* run camera_override with a step that moves the camera along path, turned
 * the way player 0 faces, looking at his eyes. mode, one of
 * camera_start_path's modes, says when the step ends. path must outlive the
 * camera's use of it */
void camera_start_path(struct path *path, int mode, Uint32 steps);
/* the boss camera's steps, which Chaos 2's camera and the CHAOS modes run
 * after they set camera_state's target. camera_boss_turn turns camera_state's
 * angles by their spins. the x and y spins move toward a quarter of the turn
 * that would aim the camera at the target. the z spin moves toward 0. unless
 * unbounded is set, a spin changes by at most camera_boss_turn_step a step
 * and stays within camera_boss_spin_limit */
void camera_boss_turn(int unbounded);
/* put camera_boss_place at player 0, moved by camera_boss_offset, which is
 * first pitched and then turned by camera_state's y angle. while player 0
 * is on land, the pitch is the angle from player 0 up to the target, and
 * otherwise 0. with below_only 1, a pitch above 0 becomes 0 */
void camera_boss_place_from_player(int below_only);
/* move camera_state's pos toward camera_boss_place, and keep the move in
 * camera_state's move. camera_boss_move_limit.x bounds its x and z, and the
 * limit's y its y. with from_anchor set, the place first moves to 30 across
 * from player 0, toward the point at x 270, z 322. it returns 0 */
int camera_boss_move(int from_anchor);
/* give E103's step work and the line through points[0] and points[1], by
 * which it frames work and player 0. work and points must outlive the
 * mode */
void camera_set_e103(struct task_work *work, NJS_POINT3 *points);
/* set the boss camera's offsets. target is the offset from what the camera
 * follows to its target. offset is the one from player 0 to
 * camera_boss_place, before camera_boss_place_from_player turns it. a null
 * takes the default. the call also puts camera_set_boss_limits's limits back to
 * their defaults */
void camera_set_boss_offsets(NJS_POINT3 *target, NJS_POINT3 *offset);
/* set the limits camera_boss_turn and camera_boss_move hold the boss camera to.
 * step bounds how much a spin changes a step, spin bounds the spins, and
 * move bounds the move a step. a null takes the default */
void camera_set_boss_limits(struct angles *step, struct angles *spin,
                            NJS_VECTOR *move);
/* set up a swing of the camera a quarter turn round the point height above
 * work. CAMERA_MODE_CHAOS_STINIT's step runs the swing, so the caller then
 * gives the camera that mode. the camera closes in from four times distance
 * to distance over 60 steps, then swings round over swing_steps steps.
 * camera_swing_done stays clear, and the pads stay off, till the swing is
 * round */
void camera_start_swing(struct task_work *work, Float height, Float distance,
                        int swing_steps);

/* the adjust modes' steps, by the modes' names. NORMAL_S and SLOW_S share
 * NORMAL's and SLOW's, and the THREE and RELATIVE modes share
 * camera_adjust_ease. each step takes work as the mode left the camera,
 * last as it was before the mode's step, and adjust, the adjust mode's own
 * record */
void camera_adjust_none(struct task_work *work, struct task_work *last,
                        struct camera_adjust_work *adjust);
void camera_adjust_normal(struct task_work *work, struct task_work *last,
                          struct camera_adjust_work *adjust);
void camera_adjust_slow(struct task_work *work, struct task_work *last,
                        struct camera_adjust_work *adjust);
void camera_adjust_time(struct task_work *work, struct task_work *last,
                        struct camera_adjust_work *adjust);
void camera_adjust_ease(struct task_work *work, struct task_work *last,
                        struct camera_adjust_work *adjust);

/* the modes and the adjust modes, and how many of each */
struct camera_mode_table {
    Sint16 mode_count;
    Sint16 adjust_count;
    struct camera_mode *modes;
    struct camera_adjust_mode *adjusts;
};

/* the modes, by number */
extern struct camera_mode camera_mode_list[];
extern struct camera_mode_table camera_mode_table;
/* the menus' collisions */
extern struct camera_layout camera_advertise_layout;

#endif

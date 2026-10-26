/* Sky Deck's stage and sky tasks, the objects sky_deck.c holds, and the
 * sounds the cannons share */
#ifndef STG06_SKY_DECK_H
#define STG06_SKY_DECK_H
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/model.h"
#include "system/task.h"

struct path;
struct path_point;

/* the stage task's step and the sky task's, which 1ST_READ.BIN's
 * stage_steps and sky_steps name */
void sky_deck_step(struct task *task);
void sky_deck_sky_step(struct task *task);
/* show Sky Deck's welcome in the message window, waiting 600 steps between
 * its fades, on the calling thread */
void sky_deck_welcome(void);

/* world_land_blocks' bit for the land's first block */
#define FIRST_BLOCK 0x1

/* the most trails at once, each trail's points, and the most volumes */
#define TRAILS 32
#define TRAIL_POINTS 5
#define TRAIL_VOLUMES 20
/* the room in each cloud layer's copy of UVs */
#define CLOUD_UV_ROOM 32

/* a point of a trail. it waits, then blows along x, faster each move up
 * to 6. its speed along y and z dies away. a move is one call of
 * trails_move */
struct trail_point {
    Sint32 wait; /* the moves it waits before it blows */
    NJS_POINT3 pos;
    NJS_VECTOR speed;
};

/* a trail volume's kind. a kind below 0 marks a free volume */
enum trail_volume_kind {
    TRAIL_UPRIGHT, /* a cylinder along y */
    TRAIL_LYING, /* a cylinder along z */
    TRAIL_BALL
};

/* a volume that turns aside the trails' points within it */
struct trail_volume {
    struct task_work *owner; /* the work of the task that set it */
    Sint32 kind;
    NJS_POINT3 center;
    /* the squared distance from center within which trails_move tests a
     * point further. it is the radius squared, plus the height squared for
     * a cylinder */
    Float reach;
    Float radius;
    Float height; /* half the cylinder's length */
};

/* the externs from here to draft_push are sky_deck.c's zeroed globals, in
 * the image's order. SHC places bss in the order of each name's first
 * declaration. so the globals only sky_deck.c reads are declared here too,
 * which keeps that order */

/* the act the stage task last started, and the act it found on its last
 * step */
extern Sint8 sky_deck_started_act;
extern Sint8 sky_deck_act;
/* each trail's points, and the moves each trail has left: below 0 for a
 * free one */
extern struct trail_point trail_points[TRAILS][TRAIL_POINTS];
extern Sint32 trail_life[TRAILS];
/* the volumes that turn the trails aside */
extern struct trail_volume trail_volumes[TRAIL_VOLUMES];
/* the fog's table and color. sky_deck_fog sets them on each step of the sky
 * task. at each frame's last vsync, frame.c's hook hands them to Ninja. it
 * reads them at their addresses in the image */
extern NJS_FOG_TABLE sky_deck_fog_table;
extern Uint32 sky_deck_fog_color;
/* the shade, from 0 to 1, by which the sky grays the fog and the back
 * color */
extern Float sky_deck_shade;
/* the fog's near and far, negative as Ninja's depths are */
extern Float sky_deck_fog_near;
extern Float sky_deck_fog_far;
/* the first act's palettes and the second's, from PL_60B.BIN and
 * PL_61B.BIN. the sky copies the first four of one into draw_palettes.
 * above the clouds' level it takes the first act's, and below it the
 * second's */
extern Uint32 sky_deck_palettes_above[DRAW_PALETTES][DRAW_PALETTE_ENTRIES * 2];
extern Uint32 sky_deck_palettes_below[DRAW_PALETTES][DRAW_PALETTE_ENTRIES * 2];
/* the sky draws only while this is 0. nothing sets it but the sky task's
 * start, to 0 */
extern int sky_deck_sky_mode;
/* the point the sky and the clouds are drawn about: the act's origin,
 * raised by sky_deck_sky_lift. the lift is the sky's rise plus 120. in the
 * first two acts, it bobs 40 either way */
extern NJS_POINT3 sky_deck_sky_pos;
extern Float sky_deck_sky_lift;
/* the value of the second act's timeline row for this step. nothing reads
 * it */
extern Float sky_deck_timeline_value;
/* how far each cloud layer's UVs have scrolled in u, from 0 to 8191 */
extern int sky_deck_cloud_scroll;
extern int sky_deck_cloud_scroll_low;
/* how far the deck rolls, about z, and pitches, about x. gravity tilts with
 * them, and sky_deck_tilt leaves each from 0 to 0xffff */
extern Angle sky_deck_roll;
extern Angle sky_deck_pitch;
/* in the second act, x moves toward sky_deck_timeline_rate by 0.05 a step.
 * y and z stay 0. nothing reads it, so the timeline moves nothing */
extern NJS_VECTOR dat_0caf373c;
/* the rate of the second act's timeline row for this step. it is read only
 * to move dat_0caf373c */
extern Float sky_deck_timeline_rate;
/* the steps into the second act's timeline, back to 0 after its last row */
extern int sky_deck_timeline_steps;
/* how many clouds live, at most 32 */
extern int sky_deck_clouds;
/* the countdown to the sky task's next cloud */
extern int sky_deck_cloud_wait;
/* each cloud layer's scrolled copy of the cloud model's UVs, a UV in each
 * color's word. the sky task writes the first 4 */
extern NJS_COLOR sky_deck_cloud_uvs[CLOUD_UV_ROOM];
extern NJS_COLOR sky_deck_cloud_uvs_low[CLOUD_UV_ROOM];
/* the first two acts' clock, in steps. the sky task counts it, back to 0
 * past 10500, and flips the deck's phase at its turns. the collapse sets it
 * to 360 as it starts */
extern int sky_deck_clock;
/* the line trails_draw hands Ninja, through one trail's points at a time */
extern NJS_POINT3COL trail_line;
extern NJS_POINT3 trail_line_points[TRAIL_POINTS];
extern NJS_COLOR trail_line_colors[TRAIL_POINTS];
/* the draft's rates, which the sky's height sets. draft_lift is the lift
 * per step the draft has built. draft_push is the push per unit of player
 * 0's height */
extern Float draft_lift;
extern Float draft_push;

/* the set pieces' steps, which the object list names. each draws its own
 * row of sky_deck_models and puts the row's world shape in the world. once
 * the collapse reaches a piece, it falls away with the land.
 * cold_fan_step, fun_0c902290 and ctrltower_step also play a motion */
void edge_0_step(struct task *task);
void branch_l_step(struct task *task);
void branch_s_step(struct task *task);
void pole_s_step(struct task *task);
void pole_l_step(struct task *task);
void base_l_step(struct task *task);
void rest_6_step(struct task *task);
void rest_45_step(struct task *task);
void block_0_step(struct task *task);
void block_1_step(struct task *task);
void block_2_step(struct task *task);
void block_3_step(struct task *task);
void block_4_step(struct task *task);
void block_5_step(struct task *task);
void cover_step(struct task *task);
void screw_step(struct task *task);
void launcher_step(struct task *task);
void block_6_step(struct task *task);
void block_7_step(struct task *task);
void rail_l_step(struct task *task);
void rail_s_step(struct task *task);
void fun_0c90226c(struct task *task);
void lantern_step(struct task *task);
void tower_i_step(struct task *task);
void rest_9_step(struct task *task);
void fun_0c902284(struct task *task);
void cold_fan_step(struct task *task);
void fun_0c902290(struct task *task);
void fun_0c902296(struct task *task);
void fun_0c90229c(struct task *task);
void fun_0c9022a2(struct task *task);
void fun_0c9022a8(struct task *task);
void fun_0c9022ae(struct task *task);
void fun_0c9022b4(struct task *task);
void carrier_step(struct task *task);
void carrier2_step(struct task *task);
void carrier3_step(struct task *task);
void fun_0c9022cc(struct task *task);
void fun_0c9022d2(struct task *task);
void fun_0c9022d8(struct task *task);
void fun_0c9022de(struct task *task);
void fun_0c9022e4(struct task *task);
void ctrltower_step(struct task *task);
void scope_step(struct task *task);
void edge_step(struct task *task);
void la_step(struct task *task);
void lb_step(struct task *task);
void lc_step(struct task *task);
void stop_step(struct task *task);
void s_wing_step(struct task *task);
void s_map_step(struct task *task);
void s_obj_step(struct task *task);
void s_eng_step(struct task *task);

/* TRUE when the task is out of range, which makes it destroy itself on its
 * next step. the task is in range while the camera or player 0 is within
 * the range of it. while player 1 is Sonic, it keeps the task in range
 * within 400. the range, squared, is the placement's, or 410 squared for a
 * task with none. a placement with PLACEMENT_IN_RANGE, or a range of 0,
 * keeps the task in range */
Bool sky_deck_try_leave(struct task *task);

/* the step the object list names for NEXT ACT. a player touching it takes
 * the stage on to the next act, and places player 0 for it */
void next_act_step(struct task *task);
/* sky_deck_change_act_step waits 60 steps, then ends the streams and takes
 * the stage on to the next act. it places player 0 for that act.
 * sky_deck_voice_change_act_step does the same, and plays a voice as it
 * starts. cannon_h.c starts the first as CANNON_H breaks, and cannon_h2.c
 * the second as CANNON_H2 breaks */
void sky_deck_voice_change_act_step(struct task *task);
void sky_deck_change_act_step(struct task *task);

/* the grips. a player near one takes hold and goes round it, unless moving
 * away from it. the player jumps off once it leaves the grip's reach or
 * stops going round. cyl_0.c, cyl_1.c and cyl_2.c each give a child
 * sky_deck_grip_start as its step. the other starts are the steps the object
 * list names for HANGPOLE, CYL_S, TANK_H and TOP_0. CYL_S turns by its ang.x
 * each step, and carries its players round with it. the other grips' ang.x
 * stays 0, so they neither turn nor carry */
void sky_deck_grip_start(struct task *task);
void hangpole_start(struct task *task);
void cyl_s_start(struct task *task);
void tank_h_start(struct task *task);
void top_0_start(struct task *task);

/* the steps the object list names for the paths players hold on to or
 * hang from. RADDER and DUCT_0 hold ledges. UNTEI, the fences and HANG_C
 * hold bars. HANG_A holds bars that turn to a ledge as its arm swings down.
 * HANG_B's step does nothing */
void radder_start(struct task *task);
void duct_start(struct task *task);
void untei_start(struct task *task);
void fence_l_start(struct task *task);
void fence_m_start(struct task *task);
void fence_s_start(struct task *task);
void hang_a_start(struct task *task);
void hang_b_start(struct task *task);
void hang_c_start(struct task *task);

/* the vars of a task that owns a path for players to hang from or hold on
 * to. points and path come from heap_alloc */
struct path_owner_vars {
    Uint8 unk_00[4];
    struct path_point *points;
    struct path *path;
};

#define PATH_OWNER(work) ((struct path_owner_vars *)&(work)->vars)

/* the trails, streaks of wind that blow along x past the deck. volumes turn
 * them aside: T_COLLI's, and the sky task's ball round player 0 */

#define TRAIL_LIFE 240 /* a new trail's life, in moves */
#define TRAIL_POINT_WAIT 10 /* the moves between its points' starts */
#define TRAIL_SPEED 3.0F /* the speed along x each point starts with */

/* free every trail and volume */
void trails_reset(void);
/* one move of every live trail: its life and each waiting point's wait
 * count down, and its other points blow on */
void trails_move(void);
/* draw every live trail as a white line through its points */
void trails_draw(void);
/* the step the object list names for TRAIL, and the step and destructor
 * it installs. the step spawns a trail where it stands. it spawns first on
 * its 31st to 60th call, then every 241 to 270 calls. it spawns none while
 * every trail is taken. the destructor does nothing */
void trail_start(struct task *task);
void trail_step(struct task *task);
void trail_destroy(struct task *task);
/* a step that spawns trails near the camera. each starts 200 short of it
 * along x, and within 100 of it along y and z. it spawns first on its 32nd
 * to 61st step, then every 1 to 20 steps. it spawns none while every trail
 * is taken */
void trail_camera_step(struct task *task);

/* give owner a volume of kind, centered at x, y, z. its radius is 10 more
 * than radius, and it reaches height above and below its middle. the call
 * takes the first volume that is free or owner's. while others hold every
 * volume, nothing changes. without force, a volume that owner holds keeps
 * its place and size, unless it is TRAIL_UPRIGHT. a freed volume keeps its
 * owner. so without force, a free one that owner held last stays free */
void trail_volume_set(struct task_work *owner, int kind, Bool force,
                      Float radius, Float height, Float x, Float y, Float z);
/* the step the object list names for T_COLLI, and the step, draw and
 * destructor it installs. the step sets the task's volume from the set
 * file. the draw shows the volume in the set editor, and the destructor
 * frees it */
void t_colli_start(struct task *task);
void t_colli_step(struct task *task);
void t_colli_draw(struct task *task);
void t_colli_destroy(struct task *task);

/* draw the scroll model the work's submode names, 0 to 11, at the work's
 * pos. it draws at 0.3 times its size. each call takes 4 from the work's timer
 * and turns its ang.z by 364. the first meshset draws through a copy of its
 * UVs. each u in the copy is an eighth of its own plus the timer. the constant
 * material's alpha is sin(ang.z) / 2 - 1/2 for the draw, and 0 after it */
void scroll_draw(struct task *task);

/* still the draft: no lift, no push and nothing built. each act's start
 * calls this */
void draft_reset(void);
/* unless act is 1, the second act, do nothing. lift player 0 by how far the
 * draft has built. push it along x while the draft has built at all. the
 * draft builds over 90 steps while player 0 is low. it dies away four times as
 * fast while player 0 is high or W_WALL is touched. it is strongest while
 * the sky's point is at 1400. past 200 either way of that, its build starts
 * over on each step, and it pushes nothing. player_work[0] must be set,
 * whatever act is */
void draft_step(Uint16 act);
/* the step the object list names for W_WALL. while its record touches
 * another, the draft dies away */
void w_wall_step(struct task *task);
/* the steps the object list names for FLAP01 and FLAP02, and the step they
 * install with the set pieces' draw and destructor. each puts its row's
 * world shape in the world where it stands */
void flap01_start(struct task *task);
void flap02_start(struct task *task);
void flap_step(struct task *task);
/* the step the object list names for HOMING, and the step and destructor
 * it installs */
void homing_start(struct task *task);
void homing_step(struct task *task);
void homing_destroy(struct task *task);

/* the sounds the cannons share */
#define SE_CANNON_FIRE 168
#define SE_CANNON_BREAK 169
#define SE_TURRET 171
#define SE_TURRET_BREAK 179

#endif

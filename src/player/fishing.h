/* the fishing game: Big's target and lure, their camera, the hud and score */
#ifndef PLAYER_FISHING_H
#define PLAYER_FISHING_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;
struct task_work;

/* in fishing_flags */
#define FISHING_HOOKED 0x1 /* a fish is on the hook */
#define FISHING_LANDED 0x2 /* the lure is back with a fish on it */
/* the hooked fish got away: the lure went too far out, or Big was hurt or
 * went under */
#define FISHING_LOST 0x4
#define FISHING_KICK 0x8 /* the fish kicks hard */
#define FISHING_BOTTOM 0x10 /* the lure rests on the bottom */
#define FISHING_PULL 0x20 /* Big pulls the rod to one side */
#define FISHING_CAMERA 0x40 /* the lure is in the water, the camera on it */
/* the fish on the hook is Hot Shelter's plug, the task at (750, 75.5,
 * -650). the lure comes back once the plug is off the hook */
#define FISHING_SHELTER_PLUG 0x80
#define FISHING_GOAL 0x400 /* Big caught the frog */
#define FISHING_SNAPPED 0x800 /* the line's tension reached 1 */
#define FISHING_TWITCH 0x1000 /* the lure twitches this step */
/* the lure's splash on the bottom plays its sprites. no other splash
 * starts till it fades */
#define FISHING_SPLASH 0x2000
/* the lure comes back with no fish: Big held both triggers, or Hot
 * Shelter's plug came off the hook */
#define FISHING_GIVE_UP 0x4000
/* Big has cast: the dangling lure takes the target's place as its goal */
#define FISHING_CAST 0x8000

/* SHC lays out zeroed globals in the order of their first declarations.
 * fishing.c's are all declared here in their order in memory, even those
 * only fishing.c reads, and its two zeroed statics follow them */

/* the weight of Big's catches this act, summed, at most 99990 */
extern int fishing_score;
/* the weight of Big's heaviest catch this act, and its kind */
extern Sint16 biggest_fish;
extern int biggest_fish_kind;
/* the weight of the stage's biggest fish in the save */
extern int fishing_record_weight;
/* the FISHING_ flags, which Big, the fishing game and the fish share */
extern Uint16 fishing_flags;
/* the tension the reel's buttons add. while a fish is hooked, each step a
 * reel button is held adds 0.0005. each step with none held takes 0.001
 * away while the strain is above 0, and so does each step Big pulls the
 * rod aside. while a fish holds the lure unhooked, every step takes 0.001
 * away while the strain is above 0 */
extern Float fishing_strain;
/* where the lure last went into the water. in a field, where player 0
 * stood at his last prize */
extern NJS_POINT3 fishing_cast_pos;
/* the steps the hooked fish has fought, which the fish counts: two for
 * each step Big pulls the rod aside. past 1800, a kick strains the line
 * less */
extern int fishing_fight_steps;

/* create the target that Big, whose task is big, aims his cast at: 20 out
 * from him along his heading, and 20 up. the result is the task, or null.
 * the target ends as he casts, or once big_not_fishing gives TRUE */
struct task *fishing_create_target(struct task *big);
/* create Big's lure at big, with its line, to dangle from his rod till he
 * casts. the task, or null */
struct task *fishing_create_lure(struct task *big);
/* the step of the hud task Big creates for player 0. outside a field, it
 * draws the score, the lives and the rings. in a field the step draws
 * nothing, and the task's draw shows a field's label and the rings. there
 * it shows the lives only while lives_add's task holds lives_shown at 0,
 * for 181 steps after a life is gained */
void fishing_hud_step(struct task *task);
/* set camera_place's target 100 from player 0, against his heading, for a
 * camera that looks at him from ahead. task goes unused */
void fishing_look_at_big(struct task *task);
/* TRUE while a task in the near list of work's player, near_tasks for
 * player 0 and near_tasks_player1 otherwise, lies within 50 of him */
Bool fishing_task_in_reach(struct task_work *work);
/* show the weight of a catch of kind, a fish's kind, then add it to the
 * score. the frog's catch saves the score too */
void fishing_show_catch(int weight, int kind);
/* the lures Big has found, from 0 to 4, which it stores in big_lure_count
 * too */
int fishing_count_lures(void);
/* set pos to the hook: the lure's position plus (0.6, -0.45, 0), turned as
 * the lure is. the origin while big_lure is null */
void fishing_hook_point(NJS_POINT3 *pos);
/* set pos to big_catch's position plus (-2, 1.5, 0), turned as the lure
 * is. the origin while big_lure is null, and otherwise big_catch must not
 * be null */
void fishing_catch_point(NJS_POINT3 *pos);
/* the water's height at the lure, as the lure last found it. while Big aims,
 * the ground's height under the target */
Float fishing_get_water(void);
/* the lure's distance from player 0, as the lure last measured it */
Float fishing_get_distance(void);
/* start a fight: show the hit banner, zero fishing_strain, and play the
 * fight's music */
void fishing_start_fight(void);
/* zero the score, the heaviest catch and fishing_cast_pos, and read
 * fishing_record_weight from the save. stage.c calls it for Big as an act
 * starts */
void fishing_reset(void);

#endif

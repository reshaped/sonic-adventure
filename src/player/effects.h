/* player effects: bubbles and ice, and Sonic's spin balls, dash, aura and
 * light charge */
#ifndef PLAYER_EFFECTS_H
#define PLAYER_EFFECTS_H
#include <ninja.h>
#include <sg_xpt.h>

struct player;
struct task;
struct task_motion;
struct task_work;

/* SON_EFF's textures: s_ball_a and b, s_dash01 to 23 and lcharge01 to 04 */
extern NJS_TEXLIST son_eff_texlist;

/* the first step of a player's bubbles, which player_init creates with the
 * player's number in work->scl.z. the bubbles take the sources of player
 * 0's character. the task ends once the player's work is gone, or on its
 * next step for a character without sources */
void bubbles_start(struct task *task);

/* the first steps of Sonic's spin balls. sonic.c creates each with the
 * player's number in PLAYER_VARS(work)->player */
void ball_jitter_start(struct task *task); /* moving with the player */
void ball_follow_start(struct task *task); /* a step behind the player */
void ball_shrink_start(struct task *task); /* left where the player was */

/* the first step of a dash, which sonic.c creates with the player's number
 * in PLAYER_VARS(work)->player. the dash is a streak from where the
 * player's middle was to where it is. without room on the heap for its
 * copy of the streak, the dash ends at once */
void dash_start(struct task *task);

/* draw the record's action on the aura's model three times, each a
 * twentieth larger than the last, in a pulsing blue. it draws under the
 * current matrix, and only on a frame's first step. work and motion go
 * unread */
void aura_draw(struct task_work *work, struct task_motion *motion,
               struct player *record);

/* a white trail with room for count points, which grows toward *follow
 * each step. follow must stay valid until the trail stops growing */
void trail_create(NJS_POINT3 *follow, int count);
/* a spark that flies from work->pos at a random speed for count steps,
 * with a trail of a random light color */
void charge_spark_create(struct task_work *work, int count);
/* one of the light charge's lights. it starts 25 to 40 from the middle of
 * the player whose work is given. it flies to that middle for count + 1
 * steps, with a trail of count points */
void charge_create(struct task_work *work, int count);

/* a block of ice around the player, which breaks into pieces once the
 * player's action changes */
void ice_create(int player);

#endif

/* the targets a falling player steers onto, what players hold, and the
 * invincibility a player is granted */
#ifndef PLAYER_TARGET_H
#define PLAYER_TARGET_H
#include <ninja.h>
#include <sg_xpt.h>

struct player;
struct task;
struct task_motion;
struct task_work;

/* make the player invincible for 120 steps, by a task of its own. player
 * must be below PLAYER_COUNT: any other reads player_work past its end */
void player_grant_invincibility(Uint8 player);
/* set or get the kind of what the player holds, which player_held_kinds
 * keeps by the player's character. requires a live player */
void player_set_held_kind(Uint8 player, Sint8 kind);
int player_get_held_kind(Uint8 player);
/* the held kinds of the eggs a Chao garden takes in: the gold egg from Station
 * Square, the silver from the Mystic Ruins and the black from the Egg Carrier.
 * a garden player 0 enters holding one takes the egg in, given an empty Chao
 * record */
#define HELD_GOLD_EGG 5
#define HELD_SILVER_EGG 10
#define HELD_BLACK_EGG 11
/* hand the player task to carry: request PLAYER_MODE_HOLD as
 * player_set_mode does, with task as the player's arg.carry. requires a
 * live player */
void player_hand_task(Uint8 player, struct task *task);
/* let go of the task the player holds, if any, and clear the task's
 * CARRY_HELD. the player's mode stays as it is */
void player_drop_held(Uint8 player);
/* give the record room for PLAYER_TARGETS targets, and none listed yet. a
 * failed allocation leaves targets null. player_targets_free frees the room
 * and nulls targets, so freeing again does nothing */
void player_targets_alloc(struct player *record);
void player_targets_free(struct player *record);
/* offer players 0 and 1 a target: a sphere with its center at x, y, z.
 * the first that qualifies lists it until its player_finish_step. a player
 * qualifies with room for it, no lower than its center, within 120 of its
 * surface */
void player_add_target(Float x, Float y, Float z, Float radius);
/* steer a falling player onto a target, in place of player_accel_air. TRUE
 * when it steers, and FALSE when it doesn't.
 *
 * it takes the nearest target, by player_add_target's distance from its
 * surface, whose center isn't above the player. a target more than 0x1800
 * off the player's facing takes part only while the player lies within its
 * radius in x and z. such a target stops the steering when it's the
 * nearest. so does a nearest target the player was inside as it was listed.
 * a pushed stick that points more than 0x1800 off the target stops it too.
 *
 * it follows the player's fall, by the player's constants, for up to 120
 * steps, and doesn't steer a fall that doesn't end within them. a shipped
 * bug's test ends the fall once the player's height above the target's
 * center drops to the center's own height. steering adds to motion's accel
 * a speed in x and z that moves the landing toward a point short of the
 * center, within the radius. that speed is spread over the fall's steps,
 * and held within ground_accel on each axis. a player facing within 0x1000
 * of the target turns toward it. then player_accel_roll runs */
Bool player_steer_to_target(struct task_work *work, struct task_motion *motion,
                            struct player *record);

#endif

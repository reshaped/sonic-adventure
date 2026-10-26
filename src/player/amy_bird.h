/* the bird that circles over Amy */
#ifndef PLAYER_AMY_BIRD_H
#define PLAYER_AMY_BIRD_H

struct task;

/* create the bird that circles player 0, and return its task. null unless
 * the game's character is Amy and game_mode is GAME_MODE_ACTION,
 * GAME_MODE_FIELD or GAME_MODE_TRIAL. null too in any act but STAGE_TWINKLE's
 * 1 and 2, STAGE_FINAL's 0, STAGE_SHELTER's 0 and 1 and the fields'. in a
 * field, null while an event has control. null when no task can be
 * created */
struct task *amy_bird_create(void);

#endif

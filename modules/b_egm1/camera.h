/* the Egg Hornet's camera. it looks halfway from the boss to player 0, from
 * near player 0 or wide of it */
#ifndef B_EGM1_CAMERA_H
#define B_EGM1_CAMERA_H

struct task;

/* place the camera near: 10 up and 40 back from player 0, as the camera
 * faces. move it there at a speed of 4. a speed of 1 moves it up to 2.5 a
 * step in x and z. in y it moves up to 1.5 */
void egg_hornet_camera_turn(void);
/* place the camera wide, 40 up and 100 back, at a speed of 1 */
void egg_hornet_camera_wide(void);
/* place the camera near, at a speed of 1 */
void egg_hornet_camera_near(void);
/* place the camera near, at a speed of 100 */
void egg_hornet_camera_snap(void);
/* aim the boss camera halfway from task's position to player 0's. it moves
 * to the place the last call above set, at its speed. task is the boss's,
 * and the boss calls it each step */
void egg_hornet_camera_follow(struct task *task);

#endif

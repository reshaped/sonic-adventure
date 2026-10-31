/* camera mode EGM3, the Egg Viper fight's camera */
#ifndef B_EGM3_CAMERA_H
#define B_EGM3_CAMERA_H
#include <ninja.h>
#include <sg_xpt.h>

struct camera_view;

/* the step of camera mode EGM3, in PHEAD, which 1ST_READ.BIN's
 * camera_mode_list names. it reads player 0's work and egg_viper_task's, so
 * both tasks must exist. it sets camera_state's phase, pos and target, and
 * leaves view alone */
void eggmobile3_camera_step(struct camera_view *view);
/* start the next step's phase from its first subphase. stop the shake too,
 * whether asked for or under way */
void egg_viper_camera_reset(void);
/* ask for a shake. the camera's next step starts it, and the view lifts from
 * the step after. an ask during a shake starts another after it */
void egg_viper_camera_shake(void);
/* move the camera speed toward pos, or onto pos once it's within speed, and
 * return TRUE then. a move leaves the camera no nearer player 0 in x and z
 * than pos is. no caller in this image */
Bool camera_move_at(NJS_POINT3 *pos, Float speed);

#endif

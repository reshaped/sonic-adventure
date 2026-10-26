/* E-101 mkII's fight camera, CAMERA_MODE_E101R. it follows player 0 and
 * turns toward the boss, gets behind the boss, or circles the boss */
#ifndef B_E101_R_CAMERA_H
#define B_E101_R_CAMERA_H
#include <ninja.h>
#include <sg_xpt.h>

/* the camera's modes. the first five follow behind player 0, looking past
 * it along the camera's heading. they differ in how the heading turns each
 * step. the aim it turns toward is the heading from player 0 to the boss */
enum e101_r_camera_mode {
    /* a tenth of the way to the aim, up to (2.5 - player 0's speed)
     * degrees. from a speed of 2.5 on, the heading holds */
    E101_R_CAMERA_FOLLOW,
    /* on by its last turn, which loses 2% each step */
    E101_R_CAMERA_COAST,
    /* a twentieth of the way to the aim, up to 3 degrees */
    E101_R_CAMERA_TURN,
    /* a twentieth of the way to the aim, up to (2 - player 0's speed)
     * degrees. the most is never less than a degree */
    E101_R_CAMERA_TRACK,
    /* as E101_R_CAMERA_FOLLOW, from 100 farther back */
    E101_R_CAMERA_BACK_OFF,
    /* ease to 75 past the boss from player 0 and 50 above that, looking back
     * at player 0 */
    E101_R_CAMERA_BEHIND,
    /* circle the boss a degree a step, 50 above it and looking at it. the
     * distance eases to 60 */
    E101_R_CAMERA_ORBIT,
    /* circle on, 2% slower each step, easing to 60 out */
    E101_R_CAMERA_ORBIT_END
};

struct camera_view;

/* CAMERA_MODE_E101R's step. it reads none of view */
void e101_r_camera_step(struct camera_view *view);

/* follow player 0 again. the camera's distance and height start at 20
 * behind and 23 above player 0, and ease from there. it stops any turn under
 * way, drops the target's rise to 0, and turns the wide view off */
void e101_r_camera_reset(void);
/* face the boss from player 0. boss, the boss's position, is kept and read
 * each step, so it must stay valid while CAMERA_MODE_E101R runs. the camera
 * starts at player 0's position. the following camera's target then stays
 * 20 above player 0's height now, plus its rise. it doesn't follow player 0
 * up or down */
void e101_r_camera_start(NJS_POINT3 *boss);
/* ease the following camera's target up to rise above its usual height.
 * the camera's own height doesn't change */
void e101_r_camera_set_target_rise(Float rise);
/* set the heading the camera turns toward, and player 0's speed across.
 * the speed limits the following camera's turn and moves it farther back. a
 * following camera replaces aim each step with the heading from player 0
 * toward the boss */
void e101_r_camera_set_aim(Angle aim, Float speed);
/* set the blast size that E101_R_CAMERA_ORBIT adds three times over to its
 * distance from the boss. the size never takes effect. the mode's step
 * clears it before reading it, and no other mode reads it */
void e101_r_camera_set_blast_size(Float size);
/* while wide is TRUE, a following camera eases to 150 or more behind
 * player 0. it eases to 60 above player 0, not 23 */
void e101_r_camera_set_wide(Bool wide);
/* mode is an enum e101_r_camera_mode */
void e101_r_camera_set_mode(int mode);
/* the size of the turn from the camera's heading to its aim, from 0 to
 * 0x8000 */
Angle e101_r_camera_get_turn(void);
/* the camera's heading, the way it faces while it follows. while it
 * circles, the heading is the way from the boss to it.
 * E101_R_CAMERA_BEHIND leaves it as it was */
Angle e101_r_camera_get_heading(void);

#endif

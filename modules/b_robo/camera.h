/* the fight's camera. it stands behind the player on a heading that turns
 * toward Zero. it circles Zero as Zero goes down */
#ifndef B_ROBO_CAMERA_H
#define B_ROBO_CAMERA_H
#include <ninja.h>
#include <sg_xpt.h>

/* how the camera turns toward the center as it follows the player */
enum erobo_camera_mode {
    EROBO_CAMERA_SLOW, /* it turns only while the player is slow */
    EROBO_CAMERA_COAST, /* its last turn dies away */
    EROBO_CAMERA_TURN, /* it turns at most 3 degrees a step */
    EROBO_CAMERA_SPEED, /* it turns faster the slower the player is */
    EROBO_CAMERA_FAR, /* as EROBO_CAMERA_TURN, from further off and higher */
    EROBO_CAMERA_CIRCLE, /* it circles the center instead */
    /* it circles the center more slowly each step */
    EROBO_CAMERA_CIRCLE_OUT
};

/* take over the game's camera, and follow the player in EROBO_CAMERA_SLOW.
 * each of the camera's steps reads the center. a caller gives one with
 * erobo_camera_set_center before the camera's first step */
void erobo_camera_start(void);
/* make center the point the camera turns toward. the camera keeps the
 * pointer, and reads the point each step. the camera starts on player 0,
 * heading toward center */
void erobo_camera_set_center(NJS_POINT3 *center);
/* set the height the camera's look moves toward. the camera sets it back
 * to 0 each step before it reads it, so it changes nothing. nothing calls
 * it */
void erobo_camera_set_lift(Float lift);
/* set how much further off the circling camera stands. EROBO_CAMERA_CIRCLE
 * sets it back to 0 */
void erobo_camera_set_reach(Float reach);
/* set the camera's mode. EROBO_CAMERA_CIRCLE also starts the circle over,
 * at its full speed and with no reach */
void erobo_camera_set_mode(enum erobo_camera_mode mode);
/* how far the camera's heading is from the player's heading to the
 * center. the camera updates the latter only while it follows the player.
 * nothing calls it */
Angle erobo_camera_center_gap(void);
/* the camera's heading. nothing calls it */
Angle erobo_camera_get_heading(void);

#endif

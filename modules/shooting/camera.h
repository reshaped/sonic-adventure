/* the sky chase's camera steps. they look along the course's path, trail
 * the tornado, or swing round a target */
#ifndef SHOOTING_CAMERA_H
#define SHOOTING_CAMERA_H
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "system/task.h"

/* the camera modes whose steps are shooting_camera_swing and
 * shooting_camera_follow, named as the camera editor names them */
#define CAMERA_MODE_BACK 46
#define CAMERA_MODE_BACK2 47

/* the trailing camera's roll, which the tornado sets as it banks */
extern Angle shooting_camera_ang_z;

/* shooting_camera_watch sits at the watch point and looks at player 0.
 * shooting_camera_look_back sits ahead on the path and looks back along it.
 * nothing in this image runs either */
void shooting_camera_watch(struct camera_view *view);
void shooting_camera_look_back(struct camera_view *view);
/* CAMERA_MODE_BACK2's step, the trailing camera. it sits behind the
 * tornado on the path and looks ahead along it */
void shooting_camera_follow(struct camera_view *view);
/* CAMERA_MODE_BACK's step, which runs shooting_camera_swing_start's swing.
 * the camera closes in on the target, then turns half way round it. it
 * follows the target throughout, even once round */
void shooting_camera_swing(struct camera_view *view);
/* give the camera CAMERA_MODE_LEAVE, which holds it where it is, looking at
 * player 0. the watch point goes ten path points behind the tornado's
 * point */
void shooting_camera_start(void);
/* set the trailing camera's spacing along the path. spacing 0 looks 10
 * points ahead of the tornado, and spacing 1 looks 5. both sit 10 points
 * behind it. any other spacing changes nothing */
void shooting_camera_set_spacing(int spacing);
/* set up a half turn of the camera round the point that lies height above
 * work's pos. the caller then gives the camera CAMERA_MODE_BACK, whose step
 * runs the swing. the camera closes in from four times distance to distance
 * over 120 steps. it then turns over swing_steps steps, at least 1. the
 * swing reads work's pos at every step, so work must outlive the mode. this
 * turns the pads off, and nothing in this image turns them on again */
void shooting_camera_swing_start(struct task_work *work, Float height,
                                 Float distance, int swing_steps);

#endif

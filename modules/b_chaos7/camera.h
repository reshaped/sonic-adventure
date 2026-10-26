/* the Perfect Chaos fight's cameras */
#ifndef B_CHAOS7_CAMERA_H
#define B_CHAOS7_CAMERA_H

#include <ninja.h>
#include <sg_xpt.h>

struct camera_view;

/* a camera shot. over its steps, the camera moves in a line from the point
 * from to the point to. its target moves from from_height to to_height above
 * where Chaos stood at the shot's first step */
struct chaos7_shot {
    NJS_POINT3 from;
    NJS_POINT3 to;
    Float from_height;
    Float to_height;
    Float steps;
};

/* set once the playing shot has ended. the next shot's first step clears it */
extern Sint8 chaos7_shot_done;
/* while set, the playing shot holds still. chaos7_shot_start clears it */
extern Sint8 chaos7_shot_held;

/* camera mode CHAOS_W's step, at 0x0c9003a0, which 1ST_READ.BIN's
 * camera_step_chaos_w jumps to (JMP at 0x8c0213da). it puts the camera
 * chaos7_camera_back from player 0, on his side away from Chaos, and
 * chaos7_camera_height above him. the camera looks at Chaos */
void chaos7_camera_entry(struct camera_view *view);

/* play shot from its start, as the camera's step at CAMERA_LEVEL_COLI. shot
 * must stay valid while it plays */
void chaos7_shot_start(struct chaos7_shot *shot);

/* a camera step that looks at player 0 from the view's pos, at his height,
 * which it writes to the view's pos.y */
void chaos7_camera_watch_step(struct camera_view *view);

#endif

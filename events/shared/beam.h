/* an event's capture beam: a cone of light that spins about its y while it
 * opens or closes. its widths are the cone's scale in x and z, and its
 * height its scale in y */
#ifndef EVENT_BEAM_H
#define EVENT_BEAM_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* what the beam's step does, by work->mode */
enum beam_mode {
    BEAM_STILL, /* nothing: it neither spins nor changes its widths */
    BEAM_OPEN, /* it spins, and widens to the widths it opens to */
    BEAM_CLOSE /* it spins, and narrows to 0 */
};

/* the beam's one texture, which each event loads from CAPTUREBEAM */
extern NJS_TEXLIST event_beam_texlist;

/* create a beam at x, y and z, turned by the angles, or return null when
 * task_create fails. its first step sets its widths to 0 and its mode to
 * BEAM_STILL, and resets its shape. set its mode or shape only after that
 * step */
struct task *event_beam_create(Float x, Float y, Float z, Angle ang_x,
                               Angle ang_y, Angle ang_z);
/* kill the beam on its next step. task may be null */
void event_beam_kill(struct task *task);
/* set the beam's mode. a mode that opens or closes it moves its widths by a
 * speed. the speed starts at 0, and gains the mode's acceleration each step */
void event_beam_set_mode(struct task *task, enum beam_mode mode);
/* set the beam's shape. width_x and width_z are the widths it opens to.
 * spin_speed is the angle it turns each step it opens or closes. a value of
 * 0 or less keeps its setting. the first step sets width_x, width_z and the
 * height to 1. it sets each acceleration to 0.01 and spin_speed to 768 */
void event_beam_set_shape(struct task *task, Float width_x, Float height,
                          Float width_z, Float open_accel, Float close_accel,
                          int spin_speed);

#endif

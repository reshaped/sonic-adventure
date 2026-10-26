/* EV0030.PRS's plane: Tails' new plane, with Tails in it */
#ifndef EV0030_PLANE_H
#define EV0030_PLANE_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

extern NJS_TEXLIST ev0030_plane_texlist;
extern NJS_TEXLIST ev0030_head_texlist;

/* create the plane, with an event record for the paths it follows. it becomes
 * the plane ev0030_plane_play acts on. its first step starts animation 0 from
 * frame 0, at half a frame a step. null when task_create fails. a failure to
 * allocate the record goes unchecked */
struct task *ev0030_plane_create(void);
/* play animation anim, 0 to 3, from frame 0 at speed frames a step. 0, 2 and
 * 3 loop, and 1 plays once and goes on to 2. the call acts on the plane last
 * created, if any. the plane's first step undoes a call made before it.
 * destroying the plane doesn't forget it, so a call after that writes into
 * freed work */
void ev0030_plane_play(int anim, Float speed);

#endif

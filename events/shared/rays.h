/* an event's rays: beams of light that spring from one point */
#ifndef EVENT_RAYS_H
#define EVENT_RAYS_H
#include <sg_xpt.h>

/* start count beams at x, y, z each step, until event_rays_kill. each beam
 * fades after time steps. it is width_foot wide in color_foot at its foot,
 * and width_tip wide in color_tip at its tip. a second create leaves the
 * first running, out of event_rays_kill's reach. when no task can be made,
 * it reads through null.
 *
 * each beam reads its foot's position from the rays' task each step.
 * event_rays_kill has that task destroy itself on its next step. a beam that
 * outlives the task then reads freed memory */
void event_rays_create(Float x, Float y, Float z, int count, int time,
                       Float width_foot, Float width_tip, Uint32 color_foot,
                       Uint32 color_tip);
void event_rays_kill(void);

#endif

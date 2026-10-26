/* Chaos's brain: CHAOS_BRAINFRAME's sixteen frames, played once at a
 * task's place, or looped at a point the brain follows */
#ifndef EFFECT_BRAIN_H
#define EFFECT_BRAIN_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

/* CHAOS_BRAINFRAME's textures, which the bosses load */
extern NJS_TEXLIST chaos_brainframe_texlist;
/* nonzero shows the looping brains. brain_create sets it, and the bosses
 * set and clear it */
extern Sint8 brain_shown;

/* a task's step that plays the brain's frames once at the task's place,
 * then destroys the task. the task needs a work */
void brain_once_step(struct task *task);
/* a looping brain at scale, offset by offset in the camera's frame from
 * follow, or from the origin when follow is null. follow and offset are
 * kept, not copied, and only read. they must outlive the task. every
 * looping brain takes the last call's scale and offset. it sets brain_shown
 * and returns the task, or null. boss is unread */
struct task *brain_create(struct task *boss, Float scale, NJS_POINT3 *follow,
                          const NJS_POINT3 *offset);

#endif

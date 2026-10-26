/* an event's Chaos puddle, and the blobs of water round a Chaos */
#ifndef CHAOS_PUDDLE_H
#define CHAOS_PUDDLE_H
#include <ninja.h>
#include <sg_xpt.h>

struct chaos;
struct task;
struct task_work;

/* the models of a Chaos's puddle and of a blob of its water */
extern NJS_OBJECT chaos_puddle;
extern NJS_OBJECT chaos_blob;
/* a sphere's model, which Chaos 0 fades along with them */
extern NJS_OBJECT chaos_sphere;

/* place count blobs, at most 16, evenly round the work. each lies a random
 * 10 to 15 out across, on the ground below it */
void blob_place(struct task_work *work, int unused, int count);
/* draw the blobs, each scaled by its size and the Chaos's scale. their
 * places across are scaled by his x scale less 0.75, but by no less than
 * 0 */
void blob_draw(void *unused, struct chaos *chaos);
/* draw the blobs as blob_draw does, but with their places across scaled by
 * half of spread times the Chaos's x scale */
void blob_draw_spread(void *unused, struct chaos *chaos, Float spread);
/* make the one puddle, facing ang, for an event to move with puddle_move.
 * returns its task. it leaves chaos_puddle's material translucent, at
 * alpha 0x99. the task and its record are allocated unchecked */
struct task *puddle_create(Angle ang);
/* move the last puddle made to pos, turning it toward there by at most 5
 * degrees a call, and set its scale. the puddle must still be live */
void puddle_move(NJS_POINT3 *pos, NJS_VECTOR *scale);

#endif

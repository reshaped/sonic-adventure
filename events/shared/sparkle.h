/* an event's sparkles: glints that drift, spin and shrink */
#ifndef EVENT_SPARKLE_H
#define EVENT_SPARKLE_H
#include <ninja.h>
#include <sg_xpt.h>

/* a sparkle in the sparkle pool at pos, moving at speed, drawn at scale.
 * each step it speeds up 2% sideways and slows 2% up and down. it turns at
 * a random rate of up to 2.5 degrees a step either way. its scale keeps
 * 59/60 of itself each step. it plays the pool's four frames over about 400
 * steps, then ends. it makes none when no particle is free, or the pool
 * wasn't made */
void event_sparkle_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale);
/* the same, but each step its scale keeps shrink of itself */
void event_sparkle_create_shrinking(NJS_POINT3 *pos, NJS_VECTOR *speed,
                                    Float scale, Float shrink);

#endif

/* Tikal's light: a ball of light that bobs at a point and sheds sparks,
 * and the red flashes it leaves behind */
#ifndef OBJECT_TIKAL_LIGHT_H
#define OBJECT_TIKAL_LIGHT_H
#include <ninja.h>

#include "system/task.h"

/* the texlist the HINT file loads into: the light's eight textures, then
 * the spark's */
extern NJS_TEXLIST hint_texlist;

/* create a light as a child of parent, bobbing at origin. origin must last
 * as long as the light, which reads it each step. holder may be NULL;
 * otherwise the light clears *holder as it's destroyed. each light made
 * sets the pace of every light, spark and flash. while an event has
 * control, one of their steps stands for two, as the event runs one step a
 * frame where a stage runs two. it returns the light, or NULL when
 * task_create_child fails */
struct task *tikal_light_create(struct task *parent, struct task **holder,
                                NJS_POINT3 *origin);
/* set the light's scale and alpha. light may be NULL */
void tikal_light_set(struct task *light, Float scale, Float alpha);
/* leave a red flash where the light is now. light may be NULL */
void tikal_light_flash(struct task *light);
/* move pos distance toward to, passing to when it's nearer. to is left
 * holding the move: the unit vector toward to, times distance */
void tikal_light_move_toward(NJS_POINT3 *pos, NJS_POINT3 *to, Float distance);

#endif

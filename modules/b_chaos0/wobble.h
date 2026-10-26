/* the node of Chaos 0's figure that wobbles and blinks */
#ifndef B_CHAOS0_WOBBLE_H
#define B_CHAOS0_WOBBLE_H
#include <ninja.h>

#include "system/task.h"

/* the node, whose model has 159 points */
extern NJS_OBJECT chaos0_wobble_node;

/* the attrflags bit of a material that chaos0_set_material draws in
 * chaos_record's second color */
#define MATERIAL_SECOND_COLOR 0x2

/* while frames is odd, make the node's first material blend its source by
 * NJD_SA_ONE. while frames is even, give that material fixed attrflags,
 * which hold no MATERIAL_SECOND_COLOR */
void chaos0_blink(int frames);
/* set Ninja's constant material by object's first material: chaos_record's
 * second color under MATERIAL_SECOND_COLOR, or else its first under
 * MATERIAL_BLINKS. it changes nothing for an object whose first material
 * has neither, or for one without a model */
void chaos0_set_material(NJS_OBJECT *object);
/* the step of the task that wobbles the node, which needs a work and a
 * motion. its first step points the node's model at a copy of its points,
 * which every step moves. its destructor puts the model's own points back.
 * the task destroys itself, and leaves the node as it was, when the heap has
 * no room for the copy or for the points' radii */
void chaos0_wobble_step(struct task *task);
/* set the wobble's size, which scales its sway, to min(size + push, 2) *
 * 0.6. it requires a live wobble task, whose first step has run and found
 * room */
void chaos0_wobble_push(Float push);

#endif

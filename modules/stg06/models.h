/* the deck's set pieces. each is a task whose work->submode picks its row of
 * sky_deck_models. the task draws the row's models and puts its world shape
 * in the world */
#ifndef STG06_MODELS_H
#define STG06_MODELS_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

struct sky_deck_model {
    NJS_OBJECT *object; /* drawn, or null */
    NJS_OBJECT *world; /* put in the world, or null */
    /* drawn under it, 1360 below sky_deck_sky_pos, or null */
    NJS_OBJECT *shadow;
    Uint8 unk_0c[4]; /* 0 in every row */
};

/* the rows, by work->submode */
extern struct sky_deck_model sky_deck_models[81];

/* a set piece's vars */
struct sky_deck_model_vars {
    /* the object world_object_alloc lent for its row's world shape, or null */
    NJS_OBJECT *world;
    Uint8 unk_04[8];
};

#define MODEL_VARS(work) ((struct sky_deck_model_vars *)&(work)->vars)

/* draw the task's row on a frame's first step only, then the row's shadow by
 * sky_deck_shadow_draw. the row's object, if any, is drawn at the task's pos
 * and turned by its angles. scl.x scales it on each axis. while work->tick
 * isn't 0, the object plays the set pieces' motion tick - 1. scl.z counts the
 * frame, up to twice the motion's frames, then starts over. past the motion's
 * last frame, the object holds that frame */
void sky_deck_model_draw(struct task *task);
/* draw object, unless null, as the task's shadow. it draws only while the task
 * lies between 1350 and 1150 below sky_deck_sky_pos. the shadow lies under the
 * task, 1360 below sky_deck_sky_pos, give or take 2 at random. it is rolled by
 * sky_deck_roll about sky_deck_sky_pos, and turned by the task's angles. its
 * constant material's alpha is -0.3 while the task lies 1220 or more below. the
 * alpha falls to -1 as the task rises to 1150 below. the draw leaves the
 * constant material 0 */
void sky_deck_shadow_draw(struct task *task, NJS_OBJECT *object);
/* put the task's row's world shape, if it has one, in the world, on the first
 * call alone. the shape stays where the task stood then. each call finds the
 * shape's model's center as the task now stands, turned and moved with it. the
 * world tests the shape while a player is within the model's r plus 50 of that
 * center */
void sky_deck_model_world(struct task *task);
/* take the task's world object, if any, out of the world. give it back to
 * world_object_alloc's pool, and set MODEL_VARS(work)->world to null */
void sky_deck_model_destroy(struct task *task);

extern NJS_TEXLIST obj_skydeck_texlist;

#endif

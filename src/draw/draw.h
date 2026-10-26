/* drawing: models, lines, and the state they draw with */
#ifndef DRAW_DRAW_H
#define DRAW_DRAW_H
#include <ninja.h>
#include <sg_xpt.h>

struct task_work;

/* draw object's tree in the current matrix, posed by motion at frame.
 * draw_action draws action->object, posed by action->motion. nothing is drawn
 * when the motion isn't null and frame isn't below its nbFrame. a null motion
 * leaves each node its own pose. keys blend linearly, an angle as a plain
 * number, not the short way round. each node turns in the order the root's
 * NJD_EVAL_ZXY_ANG gives. a node under NJD_EVAL_SKIP moves by its own pos and
 * scl instead. it turns in its own order by the three words at the stack
 * pointer, not by its ang. _nj_motion_callback_, if set, then takes the node.
 * the node's model, if any, is drawn with draw_model_nested, unless the node
 * is under NJD_EVAL_HIDE */
void draw_action(NJS_ACTION *action, Float frame);
void draw_motion(NJS_OBJECT *object, NJS_MOTION *motion, Float frame);
/* draw object's tree in the current matrix, posed by link's two motions,
 * each at the frame link gives it. the pose blends by ratio from the first
 * motion's toward the second's. draw_action_link draws link->object, posed
 * by link->motionlink. nothing is drawn when ratio is above 1, or when a
 * motion isn't null and its frame is at or above its nbFrame. a motion gives
 * a node's position, angles and scale from its keys, or the node's own where
 * it has none. a null motion gives the node's own. each of the three blends
 * the two motions' where either has keys for it, and is the node's own
 * otherwise. a scale from keys blends the motions' positions, not their
 * scales. keys blend linearly, an angle as a plain number, not the short way
 * round. each node turns in the order the root's NJD_EVAL_ZXY_ANG gives. a
 * node under NJD_EVAL_SKIP instead moves by its own pos, ang and scl,
 * turning in its own order. it moves the matrix _nj_motion_data_info_.m
 * points at, which these draws don't set: the current matrix while m is
 * null, as draw_action and draw_motion leave it. _nj_motion_callback_, if
 * set, then takes the node. the node's model, if any, is drawn with
 * draw_model, unless the node is under NJD_EVAL_HIDE */
void draw_action_link(NJS_ACTION_LINK *link, Float ratio);
void draw_motion_link(NJS_OBJECT *object, NJS_MOTION_LINK *link, Float ratio);
/* draw as draw_motion_link does, with each model morphed by shape's two
 * shape motions, each at the frame shape gives it. nothing is drawn when a
 * shape motion isn't null and its frame is at or above its nbFrame. a shape
 * motion morphs a model's points or normals from the two keys around its
 * frame. where it has no keys, it gives the model's own. where either shape
 * motion's keys give points and either's give normals, the two motions'
 * points and normals are blended by ratio. the model is then drawn with the
 * blended normals as its points, and the nbPoint vectors after them, past the
 * blends, as its normals. any other model, and one under NJD_EVAL_SHAPE_SKIP,
 * is drawn as it is. the buffer njInitShape took must hold seven NJS_POINT3
 * for each point of a morphed model. morphed normals are scaled to unit
 * length, to FSRRA's precision. a blend whose squared length is not above
 * 0.00001 becomes (0, 0, 1) */
void draw_shape_motion_link(NJS_OBJECT *object, NJS_MOTION_LINK *link,
                            NJS_MOTION_LINK *shape, Float ratio);
/* draw as draw_motion does, with each model morphed by shape at frame.
 * nothing is drawn when shape isn't null and frame isn't below its nbFrame.
 * shape morphs a model's points, normals or both where it has keys for them,
 * into the buffer njInitShape took. that buffer must hold them, the normals
 * after the points where both are morphed. a model under
 * NJD_EVAL_SHAPE_SKIP, or with no keys, is drawn as it is, and so is each
 * model when shape is null. morphed normals are scaled to unit length, to
 * FSRRA's precision. a blend whose squared length is not above 0.00001
 * becomes (0, 0, 1). unlike draw_motion's, a node under NJD_EVAL_SKIP turns by
 * its own ang */
void draw_shape_motion(NJS_OBJECT *object, NJS_MOTION *motion,
                       NJS_MOTION *shape, Float frame);
/* set the constant material, for the models that take it */
void draw_set_material(Float alpha, Float red, Float green, Float blue);
/* draw a line that shades from white at (x1, y1, z1) to pale blue at
 * (x2, y2, z2). draw_line_colored takes each end's color, and draw_line_v
 * takes the ends as points */
void draw_line(Float x1, Float y1, Float z1, Float x2, Float y2, Float z2);
void draw_line_colored(Float x1, Float y1, Float z1, const NJS_COLOR *color1,
                       Float x2, Float y2, Float z2, const NJS_COLOR *color2);
void draw_line_v(NJS_POINT3 *from, NJS_POINT3 *to);
/* set the constant material for what draws next, if characters holds the
 * bit of player 0's character, 1 << that character. the material is a gray
 * of 0.5 with alpha 0 while player 0 or 1 is within 20 of work->pos, and all
 * 0 otherwise. the caller sets it back to 0 after the draw */
void draw_highlight_near(struct task_work *work, Sint16 characters);
/* draw_highlight_near's mask for every character */
#define ALL_CHARACTERS 0xff

#endif

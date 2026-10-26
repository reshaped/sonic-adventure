/* a Chao's shape, and its drawing. the shape is the Chao's own copy of its
 * type's tree, blended from the type's six trees by its record's h_pos,
 * v_pos and growth. it wears the parts animals give it, its coat and its
 * race wins */
#ifndef AL_MAIN_SHAPE_H
#define AL_MAIN_SHAPE_H
#include <ninja.h>

#include "al_main/chao.h"
#include "animal/animal.h"
#include "system/task.h"

/* the nodes of a Chao's body that a coat covers, ending at -1 */
#define COAT_NODES {1, 3, 6, 9, 11, 13, 15, 18, 20, 22, 25, 36, 38, -1}
/* the coats, as textures of alife_chao_texlist */
#define COAT_GOLD 158 /* chao_gold */
#define COAT_PAL 161 /* pal */
/* a jewel Chao's, by its record's jewel: ame, eme, rub and saf */
#define COAT_JEWELS {217, 218, 219, 220}

/* each part's tree, by animal kind and part slot. a CHAO_CHILD takes its
 * parts from child_part_objects, and any other type from adult_part_objects.
 * of each pair only the second is read, and it is NULL where the kind gives
 * the slot no part. the part is the tree's model in the first three slots,
 * and its child in the rest. every first tree is NULL */
extern NJS_OBJECT *child_part_objects[ANIMAL_KINDS][CHAO_PART_SLOTS][2];
extern NJS_OBJECT *adult_part_objects[ANIMAL_KINDS][CHAO_PART_SLOTS][2];

/* set up the Chao's shape from its record's type, which must be CHAO_CHILD
 * to CHAO_EGG. an egg takes a child's trees. the shape's model becomes a
 * copy of the type's tree at growth 0, from the heap, which must have room
 * for it. objects lists the copy's nodes, and the six node lists the nodes
 * of the type's six trees. the Chao's texlist becomes alife_chao_texlist,
 * and its scale_speed 0. a CHAO_JEWEL Chao wears its jewel's coat. over it,
 * a CHAO_GOLD Chao wears the gold coat, or else a CHAO_PAL Chao the pal
 * coat */
void alife_shape_init(struct task_work *work, struct alife_chao *chao);
/* free the shape alife_shape_init set up, and set the shape's model to NULL.
 * first the nodes that borrow lose what they borrowed, which stays its
 * lender's: nodes 4, 7, 28, 29, 30, 33 and 34 lose their models, and the part
 * slots' child nodes take back their own children. then the copy is freed with
 * all it copied. every other node's model and child must be the copy's own */
void alife_shape_free(struct task_work *work, struct alife_chao *chao);
/* give the shape the parts its record's parts name. each part is an animal
 * kind, or -1 for none, and a Chaos Chao takes none. a slot's part is a tree
 * in its type's part table: the tree's model, or past the third slot its
 * child. where the table has a tree, the slot's node takes the part. the
 * diffuse colors of the part's materials turn white, and keep their alpha.
 * the materials are the table's, which every Chao with the part shares. the
 * first three slots read the tables at their part, not their slot, so the
 * second horn takes the first horn's tree */
void alife_shape_set_parts(struct task_work *work, struct alife_chao *chao);
/* scale node 34 by 0.3, and give it the model of the highest kind of race
 * the record has won, by race_wins. with no win it keeps its model */
void alife_shape_set_wins(struct task_work *work, struct alife_chao *chao);
/* blend the shape's copy from the type's trees by the record's h_pos, v_pos and
 * growth.
 *
 * h_pos leans from the tree at h_pos and v_pos 0 toward the run tree, or below
 * 0 toward the power tree. v_pos leans toward the fly tree, or below 0 toward
 * the swim tree. a lean of 0 counts as 0.00001. the two leans' blends mix by
 * each lean's share of their sum. growth then mixes the result with the tree at
 * growth 0.
 *
 * the points and normals of fourteen nodes blend so, or of twelve for a
 * CHAO_CHILD. their diffuse colors blend too, but the mouth's. for a color, the
 * leans' blend is kept within 0 to 255, and growth mixes it only up to 1. a
 * CHAO_BEGG Chao's colors are 85 instead. the first color of node 1 becomes the
 * icon's, with an alpha of 255. the positions of twenty-two other nodes blend
 * as the points do */
void alife_shape_blend(struct task_work *work, struct alife_chao *chao);
/* step the record's h_pos, v_pos and growth toward the Chao's own, and blend
 * the shape again where one of them rose or fell 0.003.
 *
 * the record's h_pos and v_pos are first kept within -1.5 to 1.5. each rises
 * 0.003 while below the Chao's, and then falls 0.003 while above it. a lean
 * less than 0.003 below the Chao's so rises and falls back in one call, which
 * still counts.
 *
 * status.c sets CHAO_GROWTH_DUE in the record's flags at each of its growth
 * ticks, and this function clears it. while it is set, the Chao's growth rises
 * 0.008 for a CHAO_CHILD and 0.004 otherwise, and each skill below its points
 * rises 10. each skill that rises moves the Chao's lean by 0.0015: swim moves
 * v_pos down and fly up, and run moves h_pos up and power down. each skill's
 * points then rise to the skill.
 *
 * last, the Chao's growth is kept to 1.5. the record's growth rises 0.003
 * toward a higher one, which counts, or drops to a lower one, which doesn't */
void alife_grow_step(struct task_work *work, struct alife_chao *chao);
/* spring the work's scl.y toward 1. scale_speed loses 0.15 of itself and
 * 0.28 of scl.y - 1, then adds to scl.y. scl.x becomes 2 - scl.y */
void alife_squash_step(struct task_work *work, struct alife_chao *chao);
/* step the squash, and draw the Chao as alife_chao_draw does, but at the scl
 * from before the squash. alife_far_object and alife_middle_object take its
 * place beyond 115 and 95 from the camera, not 125 and 105. alife_icon_step and
 * chao_name_step step and draw the icon and the name.
 *
 * each blend into the next motion it draws also advances: the ratio grows by
 * the next entry's blend. past 1 the ratio becomes 0, and the next motion
 * current, at its frame. the motion then takes that entry's end and a rate of
 * 1, clears MOTION_BLEND and sets MOTION_CHANGED.
 *
 * with CHAO_SHAPE_SHOWN set and CHAO_NO_FAR_TREES clear, camera_work must be
 * set */
void alife_chao_show(struct task_work *work, struct alife_chao *chao);
/* a Chao task's draw. the task must keep a struct alife_chao at its motion. it
 * draws only on a frame's first step, and only while a sphere of radius 10
 * about the work's pos is in view.
 *
 * with CHAO_SHAPE_SHOWN set, the shape draws, posed at the motion's frame or
 * blended into the next motion's. it stands at the work's pos and angles. about
 * a point 2.5 above its pos, it turns about x by unk_080 and about z by spin.
 * it is scaled by scale times the work's scl, scl.x across and scl.y up, and
 * turned a quarter turn about y.
 *
 * a CHAO_JEWEL Chao draws through draw_motion, or draw_action_link to blend,
 * and so does a CHAO_GOLD or CHAO_PAL Chao not CHAO_BEGG. any other draws
 * through njSimpleDrawMotion, or njSimpleActionLink to blend. a Chao neither
 * jewel, gold nor pal draws as alife_far_object beyond 125 from the camera, and
 * as alife_middle_object beyond 105. each takes the icon's color. with
 * CHAO_NO_FAR_TREES set, the distance counts as 0.
 *
 * with CHAO_SHAPE_SHOWN set and CHAO_IN_COCOON clear, the icon goes where node
 * 32 stands, and draws. with CHAO_IN_COCOON set, the cocoon draws at the work's
 * pos, scaled by 1.45 times cocoon_scale, with an alpha of cocoon_alpha - 1.
 * then the name draws. with CHAO_SHAPE_SHOWN set and CHAO_NO_FAR_TREES clear,
 * camera_work must be set */
void alife_chao_draw(struct task *task);

#endif

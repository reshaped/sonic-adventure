/* poses: a model tree at one frame of a motion, and the matrix of each node */
#ifndef DRAW_POSE_H
#define DRAW_POSE_H
#include <ninja.h>
#include <sg_xpt.h>

struct player;
struct player_action;
struct task_work;

/* pose the player's action at its frame, from the center of the work's
 * first collision shape, turned the way the work faces. then draw each
 * posed node's axes. the pose replaces the last, keeping at most 128 nodes */
void pose_draw_player(struct task_work *work, struct player *player,
                      struct player_action *actions);
/* pose as pose_draw_player does, but only as far as node, and draw only
 * node's axes. node must be below 128 and in the tree. returns TRUE */
Bool pose_draw_player_node(struct task_work *work, struct player *player,
                           struct player_action *actions, Uint32 node);
/* pose action at frame from base, or from the current matrix if base is
 * null, as far as node, counted from 0. the pose replaces the last, keeping
 * at most 128 nodes. out, unless null, gets node's matrix, unchecked: node
 * must be below 128 and in the tree. action, its motion and its keys must
 * be valid for frame. a motion with other than two or three factors hangs
 * the game. the current matrix is left as it was. returns TRUE, even when
 * the pose falls short of node */
Bool pose_action(NJS_MATRIX *base, NJS_ACTION *action, Float frame, Uint32 node,
                 NJS_MATRIX *out);
/* the nodes the last pose kept */
Uint32 pose_get_count(void);
/* copy node's matrix in the last pose to out, or to the current matrix if
 * out is null, and return TRUE. returns FALSE, copying nothing, if the pose
 * didn't reach node */
Bool pose_get_matrix(Uint32 node, NJS_MATRIX *out);

#endif

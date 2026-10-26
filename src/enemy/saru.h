/* the saru, a monkey enemy that throws bombs or spiked balls */
#ifndef ENEMY_SARU_H
#define ENEMY_SARU_H
#include <ninja.h>

#include "system/task.h"

/* the saru's texlist, tree and actions, for drawing it outside its own
 * task. the two prisms make a tail from node 37, the tree's last: a row of
 * body prisms that ends in an arm prism. the saru's own task draws no
 * tail */
extern NJS_TEXLIST e_saru_texlist;
extern NJS_OBJECT saru_object;
extern NJS_MODEL saru_body_prism_model;
extern NJS_MODEL saru_arm_prism_model;
/* the saru cheers with the jump or the flip. it hops, turns and waits with
 * the hop. it waits crouched, swaying between the hop's frames 2.5 and
 * 3.5 */
extern NJS_ACTION saru_jump_action;
extern NJS_ACTION saru_flip_action;
extern NJS_ACTION saru_walk_action;
extern NJS_ACTION saru_throw_action;
extern NJS_ACTION saru_hop_action;
extern NJS_ACTION saru_fidget_action;
extern NJS_TEXLIST e_bomb_texlist;
/* the spiked ball */
extern NJS_TEXLIST togeball_togeball_texlist;
extern NJS_OBJECT togeball_object;

/* the step the object lists name E SARU, which sets the saru up from its
 * set entry. the saru walks a circle round where it was placed: scl.z is
 * the circle's radius, at least 1, and ang.x its angle round it. ang.z's
 * high byte is how weak it is, 00 to ff, and its low byte the master
 * emerald piece it carries. a weaker saru waits and walks longer before it
 * faces player 0, and keeps its place more often. a scl.x below 0 keeps it
 * to the ground. scl.y picks what it throws: a bomb from 0, a spiked ball
 * below 0, and nothing from 1000 */
void saru_start(struct task *task);

#endif

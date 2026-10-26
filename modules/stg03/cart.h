/* the cart, which the set editor names E CART. player 0 boards an empty
 * cart and drives it. a monkey drives each other cart */
#ifndef STG03_CART_H
#define STG03_CART_H

#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

extern NJS_OBJECT cart_object;

/* the step twinkle_object_kinds names for E CART. its task needs a work and an
 * extra, and player 0 must exist. unless the set editor holds the cart, its
 * placement's scl.x must be 0 to 6. a cart that starts while the set editor
 * holds it gets no collision record. its later steps and cart_release then read
 * through null */
void cart_step(struct task *task);
/* ready the carts as an act starts. no cart is ridden, and camera_cart_action
 * becomes 0. Sonic's cart hovers at 5 in Twinkle Park, and at 9 elsewhere.
 * outside Twinkle Park, the act's first route becomes its cart_start_routes
 * entry, and its others cart_no_route. one task this starts notes each step
 * whether player 0's collision has a hit. another holds the world's near radius
 * at 2000 for 12 steps */
void cart_setup(void);
/* hold the cart player 0 rides, and return its task, or null when player 0
 * rides none. the held cart keeps player 0 aboard, and moves only by cart_place
 * and cart_push */
struct task *cart_hold(void);
/* while task is held or released, move it to pos, and turn the ridden cart to
 * ang. otherwise do nothing. the turn reads through null while player 0 rides
 * no cart, as after cart_release */
void cart_place(struct task *task, NJS_POINT3 *pos, struct angles *ang);
/* let task go. task must be the cart player 0 rides. player 0 rides off at
 * speed (0, 2, 2). the cart then moves only by cart_place and cart_push, and
 * its collision never registers again. in Twinkle Park, camera_restore ends the
 * cart's camera */
void cart_release(struct task *task);
/* push task along speed. a cart heads along speed, and takes it as its own
 * speed. the cart moves to speed past where it stood as its last step began. if
 * the cart is held, released, wrecked or on the rails, that step is its last
 * before then.
 *
 * a push of player 0 while it rides a cart moves and turns that cart so. the
 * cart's speed stays as it was. player 0 then moves to the cart's new place.
 * the cart's afterimages start at 0.5 unless they run. player 0 on foot takes
 * speed instead. any push of player 0 shakes pad 0. any other task stays as it
 * is */
void cart_push(struct task *task, NJS_VECTOR *speed);
/* set the boost of the cart last stepped, drawn, pushed or released, whose task
 * must still live. A adds the boost to the cart's acceleration. the boost fades
 * by 0.009 each step player 0 drives the cart, down to 0 */
void cart_set_boost(Float boost);

#endif

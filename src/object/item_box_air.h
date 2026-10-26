/* the floating item box, O ITEMBOXAIR or O ITEMBOX AIR in the object lists,
 * which a player's or an attack's touch bursts. this file also keeps two
 * things both kinds of box share. one is the row of items just collected,
 * at the foot of the screen. the other is the blast an explosion item sends
 * out */
#ifndef OBJECT_ITEM_BOX_AIR_H
#define OBJECT_ITEM_BOX_AIR_H
#include <ninja.h>
#include <sg_xpt.h>

#include "object/item_box.h"

struct task;

/* the rings the last random ring item gave. item_show keeps it with the
 * item, to draw over its face */
extern Sint32 item_rings_given;

/* make a floating box of kind at pos. with the task pool or the heap out of
 * room, it reads through a null task */
void item_box_air_create(NJS_POINT3 *pos, enum item_kind kind);
/* the object lists' step for O ITEMBOXAIR, and the step of a box
 * item_box_air_create makes. the box's kind is its scl.x, which the step
 * wraps into enum item_kind's range */
void item_box_air_step(struct task *task);
/* at a stage's start, make the task that shows the collected items, with
 * none on show and no blast. with the task pool or the heap out of room, it
 * writes through a null task */
void item_reset(void);
/* add the item to the row on show at the foot of the screen. the row must
 * hold fewer than 20 items, as a 21st writes past it. the row empties once
 * it has shrunk away */
void item_show(enum item_kind kind);
/* start an explosion item's blast at pos, in place of any blast going */
void item_explosion_start(NJS_POINT3 *pos);
/* TRUE while a blast's radius reaches past pos, and otherwise FALSE */
Bool item_explosion_reaches(NJS_POINT3 *pos);

#endif

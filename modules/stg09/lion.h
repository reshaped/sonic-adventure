/* O LION, whose sides rise and slam down in turn, and whose middle sinks and
 * rises */
#ifndef STG09_LION_H
#define STG09_LION_H

#include "system/task.h"

/* the object list's step.
 *
 * the lion starts once the player nearest it is within 400. player 0,
 * player 1 or the camera must then be at most 20 below it. its left side, at
 * -x, rises, holds and slams down. its right side does the same, and its
 * middle sinks, stays down and rises. then it waits to start again. a rising
 * side carries player 0 standing on it, and so does the middle as it rises.
 *
 * while a side holds and slams, a shape under it hurts players. a shape at
 * the middle hurts them while the middle stands more than half up. as a side
 * lands, it raises a ring of dust there. it starts a quake when player 0 is
 * within 250 of the landing's point. it also pushes player 0 away from that
 * point, and up by half the push. it pushes only while player 0 is on the
 * ground, below the point and within 250 of it.
 *
 * outside the set editor, the collision world holds the middle, the sides
 * and node 2. the world tests them while player 0 or 1 is within 336 of the
 * lion. it tests them only while player 0, player 1 or the camera is at most
 * 80 below the lion. the destructor takes them out.
 *
 * the init sets no part's height. if the cycle starts on the first run
 * step, each part's height is whatever the heap held until the part moves.
 *
 * the heap must have room for the parts, or the init writes through null.
 * it must have room for the shifted UVs while no other lion holds them, or
 * the game hangs.
 *
 * outside the set editor, the task needs a force record, and player 0 and
 * camera_work must be set. the collision world must have an object to lend,
 * or the init writes through null. the heap must have room for the
 * collision record, or the game hangs. it must have room for the record's
 * shapes, or the run writes through null. a lion the set editor holds as it
 * starts gets no collision record. it writes through null once the editor
 * lets it go */
void lion_step(struct task *task);

#endif

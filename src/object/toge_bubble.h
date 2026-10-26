/* spikes and bubbles. O TOGE is a grid of spikes that rise out of their
 * holes and sink back, over and over. under water, O BUBBLE lets go a big
 * bubble at once and then every 601 steps. a player who touches one takes
 * a breath from it. O BUBBLES lets out a stream of small ones */
#ifndef OBJECT_TOGE_BUBBLE_H
#define OBJECT_TOGE_BUBBLE_H

struct task;

/* the objects' steps in the object lists. each sets its object up and
 * hands the task on to the object's own step. bubble_init does so only once
 * it finds a surface above for the bubbles to rise to */
void toge_init(struct task *task);
void bubble_init(struct task *task);
void bubble_stream_init(struct task *task);

#endif

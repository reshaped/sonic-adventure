/* EV00BD.PRS's factory: its parts, its pieces and its machines */
#ifndef EV00BD_FACTORY_H
#define EV00BD_FACTORY_H

/* draw the factory at the current matrix, with ev00bd_factory_texlist,
 * which stays set. each machine is drawn at its frame. the frame then moves
 * on by the machine's speed, and goes back to 0 once it reaches its motion's
 * nbFrame */
void ev00bd_factory_draw(void);

#endif

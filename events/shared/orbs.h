/* an event's orbs: pieces of the Master Emerald that ring a glow over
 * player 0. they then fly to places above a fixed point, and spiral down
 * toward it */
#ifndef EVENT_ORBS_H
#define EVENT_ORBS_H

/* start count orbs, 255 at most, unless orbs have started since the last
 * event_orbs_kill. the start reseeds rand with 1. the orbs stay, unseen once
 * they finish, until event_orbs_kill */
void event_orbs_start(int count);
/* kill the orbs, if they have started since the last kill */
void event_orbs_kill(void);

#endif

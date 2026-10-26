/* act 0's floating bridge, whose planks ride the sea */
#ifndef STG01_BRIDGE_H
#define STG01_BRIDGE_H

/* create the bridge's task at 2803, -1, 365. its first step creates the two
 * ends and 25 planks as its children. it creates none while the set editor
 * holds the bridge */
void beach_bridge_create(void);
/* kill the task beach_bridge_create last made, which must still be alive,
 * or nothing if it has made none */
void beach_bridge_remove(void);

#endif

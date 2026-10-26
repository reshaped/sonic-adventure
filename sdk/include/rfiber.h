/* Shinobi fiber servers, as far as the sources use them
 *
 * a fiber is a function that runs in turn with others, each on its own stack */
#ifndef RFIBER_H
#define RFIBER_H
#include <sg_xpt.h>

/* a fiber server, whose fibers run in turn on each call to rfExecFiberServer */
struct rf_server;

/* free all eight servers. arg is not read; buInit passes 0 */
void rfInitSystem(Sint32 arg);
void rfExitSystem(void);
/* claim one of the eight servers, with fibers as its maximum fiber count, and
 * return it, or NULL when every server is in use. fibers must be at most 32,
 * a server's slots, which nothing checks */
struct rf_server *rfCreateServer(Sint32 fibers);
void rfDestroyServer(struct rf_server *server);
/* create a fiber in the server's first free slot and return the slot's
 * index. the fiber starts in func, on the size-byte stack at stack. it returns
 * -1 when the server's fiber count equals its maximum. that count starts at 0,
 * and each fiber freed takes one from it, but no call adds to it. so only a
 * server whose maximum is 0 returns -1. the caller must leave a slot free: the
 * search for one has no bound */
Sint32 rfCreateFiber(struct rf_server *server, void (*func)(void), void *stack,
                     Uint32 size);
/* rfKillFiber kills a ready fiber, which a later pass ends and frees. it
 * returns 0, or -1 for a fiber that isn't ready. rfKillFiberLevel kills each
 * ready fiber whose level is at or below level */
Sint32 rfKillFiber(struct rf_server *server, Sint32 fiber);
void rfKillFiberLevel(struct rf_server *server, Sint32 level);
void rfWakeupFiber(struct rf_server *server, Sint32 fiber);
/* run a pass over the server's fibers, then advance its tick. the pass gives
 * each ready fiber whose wake tick has come a turn, frees each ended fiber,
 * and marks each killed one ended. a turn lasts until the fiber sleeps or
 * ends, and the pass then starts again from the first fiber */
void rfExecFiberServer(struct rf_server *server);
/* each ends the running fiber's turn. rfEndFiber ends the fiber, which the
 * pass then frees. rfSleepFiber sleeps it for ticks server ticks, or until
 * rfWakeupFiber for a negative ticks. rfSleepVFiber sleeps it for one */
void rfEndFiber(void);
void rfSleepFiber(Sint32 ticks);
void rfSleepVFiber(void);

#endif

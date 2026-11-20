/* machine.h's prefetch for engine/dc/owner.c's tests on a host, the one
 * intrinsic the frame's code calls. on the SH-4 a prefetch at a store
 * queue's address sends the queue. owner_diff.c's trace records each.
 * owner_slots.c records none, and holds the machine's words by owner.c's
 * WORD */
#ifndef MACHINE_H
#define MACHINE_H

#ifdef OWNER_SLOTS
volatile KMDWORD *owner_slots_word(KMDWORD address);
#define WORD(address) (*owner_slots_word(address))
#define prefetch(address) ((void)(address))
#else
void owner_diff_prefetch(void *address);
#define prefetch(address) owner_diff_prefetch((void *)(address))
#endif

#endif

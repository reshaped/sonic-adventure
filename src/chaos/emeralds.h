/* the Chaos Emeralds that circle a Chaos and fly home */
#ifndef CHAOS_EMERALDS_H
#define CHAOS_EMERALDS_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

/* forget every emerald, leaving its task alive. chaos_setup calls this */
void chaos_emerald_clear(void);
/* make an emerald in slot, 0 to 6, which grows at home. home must outlive
 * it. returns its task, or null. an emerald already in the slot is
 * destroyed first. the new emerald's record is allocated unchecked */
struct task *chaos_emerald_create(NJS_POINT3 *home, int slot);
/* send every emerald home, but those already shrinking there */
void chaos_emerald_return_all(void);

#endif

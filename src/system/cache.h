/* operand-cache control and the matrix below Ninja's stack top */
#ifndef SYSTEM_CACHE_H
#define SYSTEM_CACHE_H
#include <sg_xpt.h>

/* each of these three returns at once and leaves the cache as it is. the
 * code past each one's RTS never runs. cache_ram_init's would turn RAM mode
 * on; cache_ram_restore's would turn it on and empty the cache;
 * cache_ram_end's would turn it off and empty the cache */
void cache_ram_init(void);
void cache_ram_restore(void);
void cache_ram_end(void);
/* invalidate every line of the operand cache, dropping dirty data without
 * writing it back. interrupts are masked while it works, and are then
 * unmasked, whatever mask they had */
void cache_empty(void);

/* load the matrix below Ninja's current one on its matrix stack into
 * XMTRX, the FPU's back bank, and return TRUE. the stack stays as it is */
Bool matrix_load_parent(void);

#endif

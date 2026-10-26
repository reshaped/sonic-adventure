/* snow bursts that the avalanche's rocks throw up where they land */
#ifndef STG08_SNOW_BURST_H
#define STG08_SNOW_BURST_H
#include <ninja.h>

/* start a snow burst at pos. the task pool and the heap must have room for
 * it: with none, it writes through null */
void snow_burst_create(NJS_POINT3 *pos);

#endif

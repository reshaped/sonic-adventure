/* dust raised along a line between two points */
#ifndef STG05_DUST_LINE_H
#define STG05_DUST_LINE_H
#include <ninja.h>
#include <sg_xpt.h>

/* raise count puffs of dust at scale on the line between from and to, in x
 * and z. the first puff is at from. each after it is 1/count of the way on
 * toward to, so none reaches to. every puff blows out across the line, to
 * the same side, and up. O DEKA raises two lines, one each way, as it lands */
void dust_create_line(NJS_POINT3 *from, NJS_POINT3 *to, Float scale,
                      Uint8 count);

#endif

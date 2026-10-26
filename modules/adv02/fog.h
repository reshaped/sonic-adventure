/* the jungle's fog. its color follows the time of day, and its table, from the
 * act's fog file, thins as the camera climbs or nears the pyramid */
#ifndef ADV02_FOG_H
#define ADV02_FOG_H
#include <ninja.h>
#include <sg_xpt.h>

/* the fog table and color mystic_ruins_set_fog, in 1ST_READ.BIN, sets at the
 * last vsync of each frame in the jungle act */
extern NJS_FOG_TABLE jungle_fog_table;
extern Uint32 jungle_fog_shown;

/* as the jungle act starts, hand the fog editor the act's fog tables, color
 * and density. then turn ninja's fog on at that density */
void jungle_fog_init(void);
/* each step, choose the fog's color for time, one of TIME_DAY to TIME_NIGHT,
 * and scale the fog's table for the camera's position. any other time keeps
 * the color last chosen. with none chosen yet, it reads the color through
 * null */
void jungle_fog_update(int time);

#endif

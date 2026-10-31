/* the texlist of the hotel act's land, and the names of its textures */
#ifndef ADV00_LAND4_TEXTURES_H
#define ADV00_LAND4_TEXTURES_H
#include <ninja.h>
#include <sg_xpt.h>

/* the texlist the hotel act's land table points at. 1ST_READ.BIN's
 * station_square_act_pvms fills its 119 textures from ADVSS04 */
extern NJS_TEXLIST station_square_land4_texlist;
/* the materials and the two arrays of strips of the hotel act's first model.
 * the model lies in the generated data after them */
extern NJS_MATERIAL dat_0ca369cc[2];
extern Sint16 dat_0ca369f4[45];
extern Sint16 dat_0ca36a4e[45];

#endif

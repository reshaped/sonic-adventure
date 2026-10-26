/* water column splash that rises from the sea and sinks back */
#ifndef STG01_WATER_COLUMN_H
#define STG01_WATER_COLUMN_H
#include <ninja.h>

/* start a column at pos, of size scale, twice scale wide. the column keeps
 * speed's x and z in its motion, where nothing reads them */
void beach_water_column_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale);

#endif

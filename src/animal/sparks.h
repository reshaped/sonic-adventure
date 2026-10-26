/* the sparks of an animal a player frees, and the flash as its barrier goes */
#ifndef ANIMAL_SPARKS_H
#define ANIMAL_SPARKS_H
#include <ninja.h>

/* the colors of the sparks and the flash. each but SPARK_MINT is 1 in its
 * own channels. in the rest it is a level that falls from 1 as the effect
 * fades, so it starts white and turns toward its own color */
enum spark_color {
    SPARK_RED,
    SPARK_GREEN,
    SPARK_BLUE,
    SPARK_WHITE,
    SPARK_YELLOW,
    SPARK_MAGENTA,
    SPARK_CYAN,
    SPARK_MINT /* a pale green throughout */
};

/* throw fourteen sparks from pos in color, a spark_color. with grounded
 * nonzero, no spark sets off downward */
void animal_sparks_create(NJS_POINT3 *pos, int color, int grounded);
/* make one flash at pos in color, a spark_color */
void animal_flash_create(NJS_POINT3 *pos, int color);

#endif

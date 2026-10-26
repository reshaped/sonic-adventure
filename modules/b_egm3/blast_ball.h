/* the blast's ball, blast_ball_object and its model */
#ifndef B_EGM3_BLAST_BALL_H
#define B_EGM3_BLAST_BALL_H

#include <ninja.h>

/* @hack type: one NJS_OBJECT declared as an array, which blast.c uses by
 * its name. declared as one NJS_OBJECT, blast.c's code runs 32 bytes long,
 * and 117 of the 294 words of blast_ball_step, blast_draw and
 * egg_viper_blast_create differ */
extern NJS_OBJECT blast_ball_object[];

#endif

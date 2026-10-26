/* the boss's blasts: a ball of fire that swells and fades. beams swing out
 * of it, and puffs burst about it and on its shell */
#ifndef B_EGM3_BLAST_H
#define B_EGM3_BLAST_H
#include <ninja.h>
#include <sg_xpt.h>

#define BLAST_COPY_POS 0x1 /* the blast keeps a copy of *pos, not pos */
#define BLAST_HURTS 0x2 /* the ball's sphere hurts what it touches */

/* a blast's times count its steps, from 1 on its first. it ends on the step
 * whose count reaches both beam_end and end */
struct blast_params {
    NJS_POINT3 *pos; /* the ball's center */
    Sint32 flags;
    /* on each step after beam_start and before beam_end, a beam swings out
     * at up to beam_speed a step. the beams' length swells to beam_length
     * and back */
    Sint32 beam_start;
    Sint32 beam_end;
    Float beam_length;
    Angle beam_speed;
    /* from grow_start, the ball and its sphere grow evenly to size by
     * grow_end. the ball then swells on ever slower, while its sphere stays
     * at size */
    Sint32 grow_start;
    Sint32 grow_end;
    /* after fade_start, the ball fades to nothing at end. each step also
     * takes half a percent off its size */
    Sint32 fade_start;
    Sint32 end;
    Float size; /* a radius */
};

/* make a blast from a copy of *params, or none when no task or memory is
 * free. without BLAST_COPY_POS, the blast and its beams keep pos, which
 * must outlive them. beam_end must differ from beam_start, since the blast
 * divides by their difference */
void egg_viper_blast_create(struct blast_params *params);

#endif

/* the stage's blasts: a ball of fire that swells and fades, with beams
 * swinging out of it and puffs about it */
#ifndef B_EGM2_BLAST_H
#define B_EGM2_BLAST_H
#include <ninja.h>
#include <sg_xpt.h>

#define BLAST_COPY_POS 0x1 /* the blast keeps a copy of *pos, not pos */
/* the ball's sphere hurts the players and objects it touches */
#define BLAST_HURTS 0x2

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
 * must outlive them. it divides half a turn by beam_end - beam_start. the
 * bombs, missiles and cars leave beam_start and beam_end 0. SHC's division
 * by 0 then returns 0 and sets errno to 1102 */
void egg_walker_blast_create(struct blast_params *params);

#endif

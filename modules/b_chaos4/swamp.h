/* Chaos 4's swamp: its waves and the ripples that move them, the calm
 * surface they fade with, and the bottom below them */
#ifndef B_CHAOS4_SWAMP_H
#define B_CHAOS4_SWAMP_H
#include <ninja.h>
#include <sg_xpt.h>

/* the height of the swamp's surface */
#define SWAMP_Y 20.0F

/* a ring spreading out across the waves from pos. its radius grows and its
 * strength fades each frame, but not for Knuckles. its slot is free while
 * strength is 0 or less */
struct swamp_ripple {
    NJS_POINT3 pos;
    Float radius;
    Float strength;
};

#define SWAMP_RIPPLES 16

/* the waves' record, the swamp's task's extra. speeds, offsets and rests
 * each hold a Float for each point of the waves' model. they are the
 * point's speed in y, its offset from its rest and its rest height */
struct swamp {
    /* waves_object's copy in the collision world, with a model and points
     * of its own */
    NJS_OBJECT *waves;
    /* another copy in the world, 0.1 higher, on the waves' model. the
     * water is what the swamp draws */
    NJS_OBJECT *water;
    struct swamp_ripple ripples[SWAMP_RIPPLES];
    Uint8 unk_148[0xc];
    NJS_ARGB material; /* the water's constant material */
    Float *speeds;
    Float *offsets;
    Float *rests;
};

/* swamp_current is the waves' record, and swamp_ripples its ripples. both
 * are NULL while there are no waves. only swamp.c reads swamp_ripples. keep
 * its declaration here, ahead of swamp_current's: SHC lays out bss in the
 * order of each name's first declaration */
extern struct swamp_ripple *swamp_ripples;
extern struct swamp *swamp_current;

/* start the waves' task, then the calm surface's. the calm surface shows,
 * and the water stays hidden until swamp_fade(1). when the waves' task
 * can't allocate its arrays, it ends. when it can't allocate its record, it
 * runs on with none, and its step reads through the NULL. either way the
 * calm surface doesn't start */
void swamp_create(void);
/* start a ripple at pos, copied, in the first free slot. it starts none
 * without the waves, or while all SWAMP_RIPPLES slots are taken */
void swamp_ripple_add(NJS_POINT3 *pos);
/* fade from the calm surface to the water, or back for to_waves 0. the
 * fade first shows the side it leaves and hides the other, whatever showed
 * before. it takes 34 steps: two halves of 16, between a first and a last */
void swamp_fade(int to_waves);
/* start the bottom's task, which puts the swamp's solid bottom in the
 * world. each step that finds player 0 below y 0, it destroys the music's
 * streams and starts player 0's fall out of the stage */
void swamp_bottom_create(void);

#endif

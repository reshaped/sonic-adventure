/* an event's sprites: texlists drawn as additive 3d sprites, each a task in
 * a slot */
#ifndef EVENT_SPRITE_H
#define EVENT_SPRITE_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the number of slots. a slot holds the sprite last made in it, until
 * event_sprite_kill. no call checks that slot lies from 0 to
 * EVENT_SPRITE_SLOTS - 1 */
#define EVENT_SPRITE_SLOTS 20

/* a looping sprite's scale, to which event_sprite_set_scale's adds */
#define EVENT_SPRITE_LOOP_SCALE 10.0F
/* a following sprite grows while both sides of its scale lie between 0 and
 * this */
#define EVENT_SPRITE_GROWTH_LIMIT 100.0F

/* the texlists in event_sprite_texlists: runs of EFF_REGULAR's textures,
 * and one list of the sprites' own */
enum event_sprite_texlist {
    EVENT_SPRITE_RING,
    EVENT_SPRITE_DUST,
    EVENT_SPRITE_BOMB,
    EVENT_SPRITE_EXPLOSION,
    EVENT_SPRITE_FLAME,
    EVENT_SPRITE_LENS,
    EVENT_SPRITE_PANLIGHT, /* the texture panlight, then EVENT_SPRITE_HIT's
                            * first seven */
    EVENT_SPRITE_HIT,
    EVENT_SPRITE_SPARKLE,
    EVENT_SPRITE_LIGHTNING,
    EVENT_SPRITE_SPLASH,
    EVENT_SPRITE_THRUST,
    EVENT_SPRITE_SNOW,
    EVENT_SPRITE_BLAST,
    EVENT_SPRITE_BUBBLE,
    EVENT_SPRITE_SPLASH_PVR, /* EVENT_SPRITE_SPLASH's textures, each name
                              * ending in .pvr */
    EVENT_SPRITE_MAGNETIC_BARRIER,
    EVENT_SPRITE_BARRIER,
    EVENT_SPRITE_INVINCIBLE,
    EVENT_SPRITE_JET,
    EVENT_SPRITE_BLINK,
    EVENT_SPRITE_INVINCIBLE_BALL,
    EVENT_SPRITE_TEXLISTS
};

extern NJS_TEXLIST event_sprite_texlists[EVENT_SPRITE_TEXLISTS];

/* load texlist's textures, and return what njLoadTexture returns */
Sint32 event_sprite_load_texture(NJS_TEXLIST *texlist);
/* destroy the sprite in slot now, if there is one */
void event_sprite_kill(Sint16 slot);
/* a looping sprite in slot. it stands at the offset x, y, z from base,
 * turned by base's angles as they are now. with base null, it stands at the
 * offset itself. it shows frames 1 to 7 and then 0 of texlist, a step each.
 * then it shows nothing for a step, and starts over. its scale is
 * EVENT_SPRITE_LOOP_SCALE plus what event_sprite_set_scale sets, with x and
 * y swapped. a sprite already in slot runs on, out of reach. when no task
 * can be made, the slot is left empty */
void event_sprite_create_loop(struct task *base, Sint16 slot, Float x, Float y,
                              Float z, NJS_TEXLIST *texlist);
/* a following sprite in slot. it keeps to the offset x, y, z from base,
 * turned by base's angles as they change. base must outlive it. with base
 * null, it stays at the offset itself. it moves through texlist's
 * frames, one a step to start, and starts over past the last. it starts
 * white, at a scale of 1 and an angle of 0. a sprite already in slot runs
 * on, out of reach. when no task can be made, the slot is left empty. the
 * record it allocated for the sprite then leaks. when the heap can't give
 * that record, it stores through null */
void event_sprite_create(struct task *base, Sint16 slot, Float x, Float y,
                         Float z, NJS_TEXLIST *texlist);
/* set the scale of the sprite in slot, of either kind, if there is one */
void event_sprite_set_scale(Sint16 slot, Float x, Float y);

/* the calls below change the following sprite in slot, if there is one. on
 * a looping sprite, they store through its null record */

/* set its color */
void event_sprite_set_color(Sint16 slot, Float alpha, Float red, Float green,
                            Float blue);
/* add ang to its angle, and make spin its turn each step */
void event_sprite_turn(Sint16 slot, Angle ang, Angle spin);
/* set what each step adds to its scale's x and y, while both lie between 0
 * and EVENT_SPRITE_GROWTH_LIMIT */
void event_sprite_set_growth(Sint16 slot, Float x, Float y);
/* set the frames it moves through a step. above 1, it still moves one */
void event_sprite_set_speed(Sint16 slot, Float speed);

/* do nothing */
void event_sprite_nop(int unused);

#endif

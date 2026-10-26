/* Knuckles' effects: his punches, digging and radar, and his maximum heat,
 * the attack his fighting gloves charge */
#ifndef PLAYER_KNUCKLES_EFFECTS_H
#define PLAYER_KNUCKLES_EFFECTS_H
#include <ninja.h>
#include <sg_xpt.h>

struct angles;
struct task;
struct task_work;

/* the steps of a punch's tasks, each made with TASK_WORK and starting at
 * its work's pos. the flash plays where his fist lands. the wave moves by the
 * work's scl each step and shrinks away over about 12 steps, its collision
 * sphere with it */
void knuckles_fist_flash_step(struct task *task);
void knuckles_punch_wave_step(struct task *task);
/* the step of a puff made at the work's pos. it drifts off a random way,
 * slowing, and grows and fades for about 150 steps */
void knuckles_puff_step(struct task *task);
/* ten glows burst from pos, then slow and fade out over 30 steps. unused is
 * never read */
void knuckles_make_burst(NJS_POINT3 *pos, int unused);
/* the step of a clod of dirt, thrown from the work's pos at the work's scl.
 * it falls until it drops below a random height up to 1 under its start,
 * then sinks. it ends after 48 steps */
void knuckles_dirt_step(struct task *task);
/* draw sprite, one of the radar's, at pos's x and y on the screen, at scale
 * and alpha */
void knuckles_draw_radar_sprite(NJS_POINT3 *pos, int sprite, Float scale,
                                Float alpha);
/* four chips of rock fly from pos at speed, each with a random spread up to
 * 0.5 either way on each axis. each tumbles as it falls, for 60 steps */
void knuckles_scatter_rocks(NJS_POINT3 *pos, NJS_VECTOR *speed);
/* a fountain at pos of what his digging turns up: 32 rings, up to 5 every
 * 17 steps, and dirt and rock every eighth step */
void knuckles_make_fountain(NJS_POINT3 *pos);
/* a mote of his maximum heat circles player 0 for about 18 steps, fading.
 * charge, from 0 to 1, sets its alpha and how fast it circles */
void knuckles_make_mote(Float charge);
/* draw his maximum heat's aura at pos, at pulse from 0 to 1, which sets its
 * size, alpha and turn */
void knuckles_draw_aura(NJS_POINT3 *pos, Float pulse);
/* a pulse of the aura round player 0, which fades as it goes.
 * knuckles_pulse_in's shrinks onto him over 18 steps, and
 * knuckles_pulse_out's grows from him over 24 */
void knuckles_pulse_in(void);
void knuckles_pulse_out(void);
/* draw the arc of his third punch at pos, turned by ang, at a frame of the
 * punch from 20 to 30 */
void knuckles_draw_swing(NJS_POINT3 *pos, struct angles *ang, Float frame);
/* his maximum heat's trail of up to 16 rings, newest first.
 * knuckles_trail_draw adds a ring round work and draws the trail, and
 * knuckles_trail_clear empties it */
void knuckles_trail_clear(void);
void knuckles_trail_draw(struct task_work *work);

#endif

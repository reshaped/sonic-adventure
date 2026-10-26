/* E-101 mkII's effects, from lightning and puffs of thrust to its jets'
 * flames and its sparks */
#ifndef B_E101_R_EFFECTS_H
#define B_E101_R_EFFECTS_H
#include <ninja.h>
#include <sg_xpt.h>

/* make a task that crackles with lightning round pos, and ends on its 60th
 * step. until then, each frame's first step adds a bolt up to 9 from pos
 * along each axis. the task keeps pos, which must outlive it */
void e101_mk2_lightning_create(NJS_POINT3 *pos);
/* a bolt of lightning up to range from pos along each axis */
void e101_mk2_lightning_near(NJS_POINT3 *pos, Float range);
/* a glowing puff of thrust, up to 1 from pos in x and z. it moves at dir,
 * give or take up to 1 along each axis */
void e101_mk2_thrust_puff(NJS_POINT3 *pos, NJS_VECTOR *dir);
/* a beam of light length long from pos, swinging at random. the beam keeps
 * pos, which must outlive it. when no task can be made, beam_create stores
 * through null */
void e101_mk2_light_burst(NJS_POINT3 *pos, Float length);
/* a bomb's white puff up to range from pos along each axis. its scale is
 * random, from 0.3 to 0.7 */
void e101_mk2_bomb_near(NJS_POINT3 *pos, Float range);
/* bind the textures of the beam's sparks and make their pool, which
 * e101_mk2_beam_sparks_clear kills */
void e101_mk2_beam_sparks_init(void);
void e101_mk2_beam_sparks_clear(void);
/* add three sparks to the pool, each up to 20 from pos along each axis.
 * every spark in the pool then closes on pos. it reads the pool unchecked,
 * so the pool must exist: made by init, and not yet killed */
void e101_mk2_beam_sparks(NJS_POINT3 *pos);
/* make the jets' task, unless it runs already. it draws a glow at pos[0]
 * and pos[1], and from each a flame along dirs[0] and dirs[1]. the task
 * keeps pos and dirs, which must outlive it. it starts with no flames */
void e101_mk2_jets_create(NJS_POINT3 *pos, NJS_VECTOR *dirs);
/* kill the jets' task, if it runs */
void e101_mk2_jets_destroy(void);
/* what e101_mk2_jets_set_state asks of the flames: to flare up, or to die
 * down. flaring, they grow by half each step, up to 30 times dirs */
#define JETS_FLARE 1
#define JETS_SETTLE 2
/* ask the flames to flare or settle from the task's next step, if it runs */
void e101_mk2_jets_set_state(int state);
/* bind the textures of the swat's flash and sparks, and make the sparks'
 * pool, which e101_mk2_swat_sparks_clear kills */
void e101_mk2_swat_sparks_init(void);
void e101_mk2_swat_sparks_clear(void);
/* a flash at pos, and fifteen sparks flying out from it. it reads the
 * sparks' pool unchecked, so the pool must exist: made by init, and not yet
 * killed */
void e101_mk2_swat_sparks(NJS_POINT3 *pos);

#endif

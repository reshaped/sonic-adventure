/* the light and smoke of Zero's fight. the glow and the sparkles last from
 * erobo_effects_start to erobo_effects_end. each other effect is made or
 * drawn by one call. the zaps, the chains and the sparks draw with textures
 * erobo_effects_start binds */
#ifndef B_ROBO_EFFECTS_H
#define B_ROBO_EFFECTS_H
#include <ninja.h>
#include <sg_xpt.h>

#include "effect/sprite.h"
#include "system/task.h"

/* how the glow's alpha moves each step. the glow starts in
 * EROBO_GLOW_STILL at an alpha of 0, and draws while its alpha isn't 0 */
enum erobo_glow_mode {
    EROBO_GLOW_STILL, /* the alpha holds */
    EROBO_GLOW_IN, /* to 0.6: up by 0.02 a step, or down at once */
    EROBO_GLOW_BRIGHT, /* up to 1, by 0.1 a step */
    EROBO_GLOW_OUT /* down to 0, by 0.04 a step, then EROBO_GLOW_STILL */
};

/* how the sparkles' scale moves each step. they start hidden, at a scale
 * of 0 */
enum erobo_sparkles_mode {
    EROBO_SPARKLES_HIDDEN, /* not drawn, and the scale holds */
    EROBO_SPARKLES_IN, /* up to 1, by 0.04 a step */
    EROBO_SPARKLES_OUT /* down to 0, by 0.02 a step, then hidden */
};

/* start Zero's last explosion at pos, which must outlive it. returns the
 * steps the explosion lasts */
int erobo_explosion_start(NJS_POINT3 *pos);
/* make a beam that swings out from pos for 20 steps, then fades. it grows
 * until it is length long. it turns at random, by up to speed a step about
 * each axis. pos must outlive the beam. a length of 0 or less makes none */
void erobo_beams_create(NJS_POINT3 *pos, Float length, Angle speed);
/* make a puff of dust at scale, within 1 of pos in x and z. it sets off at
 * a random speed of up to 1 along each axis */
void erobo_dust_create(NJS_POINT3 *pos, Float scale);
/* make lightning, or a bomb's puff at scale, at a random point near pos.
 * the point lies within range of pos in x and in z. it lies from 8 below pos
 * to 10 above */
void erobo_lightning_create(NJS_POINT3 *pos, Float range);
void erobo_bomb_create(NJS_POINT3 *pos, Float range, Float scale);
/* set the glow's mode. it does nothing while there is no glow */
void erobo_glow_set_mode(enum erobo_glow_mode mode);
/* make a zap, a streak of light that lasts eight frames. it joins two
 * points radius from a center in x and z. the center lies from 8 below pos
 * to 10 above. the second point lies up to 30 degrees round from the first.
 * it lies from 7 below the first to 7 above */
void erobo_zap_create(NJS_POINT3 *pos, Float radius);
/* set the sparkles' mode, or the two points they draw at. each does nothing
 * while there are no sparkles. the points are kept, not copied, and the
 * sparkles draw at them in every mode but EROBO_SPARKLES_HIDDEN. the
 * sparkles start with none, so a caller sets them before any other mode */
void erobo_sparkles_set_mode(enum erobo_sparkles_mode mode);
void erobo_sparkles_set_points(NJS_POINT3 *points);
/* draw a chain of light between from and to in count equal links. a count
 * below 1 draws none. each link is 3 * scale wide. frame picks one of twelve
 * looks, frame mod 12, and mustn't be negative. the chain's six textures
 * hold two looks each, one in each half */
void erobo_chain_draw(NJS_POINT3 *from, NJS_POINT3 *to, int frame, int count,
                      Float scale);
/* set sprite up as one of Zero's sparks at pos and scale, as sprite_set does
 * with flags */
void erobo_spark_set(struct sprite *sprite, NJS_POINT3 *pos, Float scale,
                     int flags);
/* make the glow and the sparkles, unless each is there already. then bind
 * the textures of the zaps, the chains and the sparks. the glow follows
 * task's work pos, which must last until erobo_effects_end. erobo_load must
 * have loaded erobo_textures */
void erobo_effects_start(struct task *task);
/* kill the glow's task and the sparkles' */
void erobo_effects_end(void);

#endif

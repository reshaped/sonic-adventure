/* EV00FA.PRS's splash beside the Tornado 2: ripples, a column of water and
 * rings of spray */
#ifndef EV00FA_SPLASH_H
#define EV00FA_SPLASH_H
#include <ninja.h>
#include <sg_xpt.h>

/* ev00fa_splash_texlist, which the event loads from L_SIBUKI and the splash
 * draws with */
extern NJS_TEXLIST ev00fa_splash_texlist;

/* start a splash at x, y, z, each of its parts scaled by scale. its first
 * step makes ripples, and then it counts its steps. it makes a column of
 * water at a count of 8 / speed. from 20 / speed on, it makes a ring of
 * spray at each multiple of 5 / speed. it ends at 60 / speed, and its parts
 * play on to their own ends. each of these quotients is truncated. the
 * ripples and rings advance speed motion frames a step, and the column
 * widens speed times as fast as at a speed of 1. speed must be above 0 and
 * at most 5: above 5, 5 / speed truncates to 0, and the ring test divides
 * by it */
void ev00fa_splash_create(Float x, Float y, Float z, Float speed, Float scale);
/* kill the last splash created, if its slot holds one, and clear the slot.
 * a splash that ends leaves the slot set. a later call then writes
 * task_destroy into the step of whatever lies at that address, which kills
 * any task the pool has since made there */
void ev00fa_splash_kill(void);

#endif

/* the shots the sweeping turrets fire, which fall onto the deck */
#ifndef STG06_SHOTS_H
#define STG06_SHOTS_H
#include <ninja.h>
#include <sg_xpt.h>

/* the shots' texlist. 1ST_READ.BIN's sky_deck_pvms names it with no pvm,
 * so texture_load_stage loads it with njLoadTexture */
extern NJS_TEXLIST shots_texlist;

/* fire a shot from from to land on to, which must be lower. the shot falls from
 * rest, 0.01 faster each step. it moves in x and z at a steady speed, so that
 * it comes down about on to. all shots share one task, which the first shot
 * fired while there is none makes at from. return 1, or 0 when the task or the
 * shot can't be allocated. scale is never read */
int sky_deck_shot_fire(NJS_POINT3 *from, NJS_POINT3 *to, Float scale);

#endif

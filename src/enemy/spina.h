/* the spina, a flier with a spinning blade on top, in three kinds */
#ifndef ENEMY_SPINA_H
#define ENEMY_SPINA_H
#include <ninja.h>

struct task;
struct task_work;

/* SUPI_SUPI, the spinas' PVM */
extern NJS_TEXLIST supi_supi_texlist;

/* the steps the object lists name SPINA A, B and C. ang.z's low byte is
 * the master emerald piece the spina carries, which Knuckles takes as it
 * explodes. a spina farther than scl.x + 60 from where it was placed turns
 * toward that place.
 *
 * SPINA A flies ahead on its heading. it turns toward player 0 while it
 * sees a player. its height drifts toward 6 above player 0's, held within
 * 20 of the height it was placed at.
 *
 * SPINA B and SPINA C swing between home - scl and home + scl, home being
 * where they were placed. ang.x is the swing's phase. the swing's speed is
 * ang.z >> 8, in angle units a step. SPINA C throws sparks now and then */
void spina_a_step(struct task *task);
void spina_b_step(struct task *task);
void spina_c_step(struct task *task);

/* draw work's shadow, and hold work no more than 3 above the floor
 * shadow_draw finds */
void spina_keep_low(struct task_work *work);

#endif

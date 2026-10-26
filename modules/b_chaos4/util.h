/* helpers Chaos 4's files share: his turn, his emeralds, the splash and
 * its sprites, and Tikal's hint */
#ifndef B_CHAOS4_UTIL_H
#define B_CHAOS4_UTIL_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* chaos4_splash_sprite is the splash, which chaos4_splash_create sets up
 * again for each splash. chaos4_drop_sprite is a drop on the CHAOS_EFFECT
 * pvm's textures, which split.c's flowing drops draw. keep the two in this
 * order: SHC lays out bss in the order of each name's first declaration */
extern NJS_SPRITE chaos4_splash_sprite;
extern NJS_SPRITE chaos4_drop_sprite;

/* turn work's y angle three tenths of the way toward what work's record's
 * turn names, the short way round. TURN_NONE, or a value outside the enum,
 * leaves it as it is */
void chaos4_turn(struct task_work *work);
/* set Ninja's constant material to 0 in every channel */
void chaos4_material_reset(void);
/* make an emerald in each of slots 0 to 3, growing at the boss point of
 * its slot's number. an emerald already in one of those slots is destroyed
 * first */
void chaos4_emeralds_create(void);
/* set up both sprites on their textures. only this call sets up
 * chaos4_drop_sprite */
void chaos4_sprites_init(void);
/* play the splash's frames once at pos, still */
void chaos4_splash_create(NJS_POINT3 *pos);
/* show Tikal's hint, once while the module stays loaded. it shows after
 * the stage's first miss, retry or restart. it also shows once the time
 * passes a minute while the boss keeps the 5 health he starts with. against
 * Tails he loses 1 of those before chaos4_hint first runs, so only a miss, a
 * retry or a restart shows it. chaos4_hint_end ends the hint
 * tikal_hint_create last made */
void chaos4_hint(void);
void chaos4_hint_end(void);

/* the CHAOS4_SHIBUKI pvm's textures, the splash's frames, which
 * chaos4_pvms loads */
extern NJS_TEXLIST chaos4_shibuki_texlist;

#endif

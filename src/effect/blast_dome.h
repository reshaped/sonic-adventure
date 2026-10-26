/* the blast dome: a ball of bands that can scroll its texture, thin its
 * bands and fade. the saru's bomb, the gacha robot and the events' dome draw
 * it */
#ifndef EFFECT_BLAST_DOME_H
#define EFFECT_BLAST_DOME_H
#include <ninja.h>
#include <sg_xpt.h>

/* p_exp, the dome's texture */
extern NJS_TEXLIST blast_dome_texlist;

/* make the dome's model on the first call. later calls do nothing */
void blast_dome_init(void);
/* set the texture's v offset to scroll, from where it lies at 0 rather than
 * from the last scroll */
void blast_dome_scroll(int scroll);
/* place each band's top ring at blend times the way from its bottom ring
 * to its own place. 1 leaves the band whole, and 0 flattens it onto its
 * bottom ring */
void blast_dome_blend(Float blend);
/* set the dome's alpha, clamped to 0 to 1 */
void blast_dome_set_alpha(Float alpha);
void blast_dome_draw(void);

#endif

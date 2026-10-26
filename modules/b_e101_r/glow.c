/* E-101 mkII's glow, an object of its model file. it turns through eight of
 * the boss's textures, and adds its light to what's behind it */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_e101_r/files.h"
#include "b_e101_r/glow.h"
#include "draw/draw.h"
#include "system/frame.h"

/* the glow's object, as a byte offset in E101R.NB */
#define GLOW_OBJECT 0xa19c
/* the glow's textures in e101_r_texlist: GLOW_TEXTURES from GLOW_TEXTURE,
 * two steps each */
#define GLOW_TEXTURE 9
#define GLOW_TEXTURES 8

/* the object e101_mk2_glow_draw draws, null before init and after clear */
static NJS_OBJECT *glow_object = NULL;

static Float glow_fade; /* how far it has faded in, from 0 to 1 */
static Bool glow_on; /* whether it fades in or out */
static int glow_steps; /* the steps e101_mk2_glow_step has run */

void e101_mk2_glow_init(void)
{
    glow_fade = 0.0F;
    glow_on = FALSE;
    glow_steps = 0;
    glow_object = (NJS_OBJECT *)(e101_r_models + GLOW_OBJECT);
}

void e101_mk2_glow_clear(void)
{
    glow_object = NULL;
}

void e101_mk2_glow_show(Bool on)
{
    glow_on = on;
}

void e101_mk2_glow_step(void)
{
    ((NJS_OBJECT *)(e101_r_models + GLOW_OBJECT))->model->mats[0].attr_texId =
        ((++glow_steps >> 1) % GLOW_TEXTURES) + GLOW_TEXTURE;
    switch (glow_on) {
    case TRUE:
        if (glow_fade < 1.0F) {
            glow_fade += 0.02F;
            if (glow_fade >= 1.0F) {
                glow_fade = 1.0F;
            }
        }
        break;
    case FALSE:
        if (glow_fade > 0.0F) {
            glow_fade -= 0.05F;
            if (glow_fade <= 0.0F) {
                glow_fade = 0.0F;
            }
        }
        break;
    }
}

/* the glow draws unlit, with half its fade as its alpha */
void e101_mk2_glow_draw(void)
{
    if (glow_fade != 0.0F && frame_step == 0 && glow_object != NULL) {
        njControl3D(NJD_CONTROL_3D_CONSTANT_ATTR |
                    NJD_CONTROL_3D_CONSTANT_MATERIAL);
        njSetConstantAttr(~(NJD_SA_MASK | NJD_DA_MASK),
                          NJD_SA_SRC | NJD_DA_ONE | NJD_FLAG_IGNORE_LIGHT);
        draw_set_material(glow_fade * 0.5F, 1.0F, 1.0F, 1.0F);
        njSetTexture(e101_r_texlist);
        njDrawObject(glow_object);
    }
}

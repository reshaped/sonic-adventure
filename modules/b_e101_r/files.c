/* E-101 mkII's files. E101R.PB's textures and E101R.NB's models and motions
 * load into the memory past the module's end. the boss's actions and its
 * arms' trail objects point into E101R.NB */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_e101_r/files.h"
#include "system/binary.h"
#include "system/loader.h"

/* the end of the module's bss, a word of bounds.src's table */
extern Uint8 *e101_r_bss_end;

static struct njb_action dash_offsets = {
#include "b_e101_r/files/dash_offsets.inc"
};
static struct njb_action swat1_offsets = {
#include "b_e101_r/files/swat1_offsets.inc"
};
static struct njb_action swat0_offsets = {
#include "b_e101_r/files/swat0_offsets.inc"
};
static struct njb_action dash_end_offsets = {
#include "b_e101_r/files/dash_end_offsets.inc"
};
static struct njb_action hurt_offsets = {
#include "b_e101_r/files/hurt_offsets.inc"
};
static struct njb_action hurt_dashing_offsets = {
#include "b_e101_r/files/hurt_dashing_offsets.inc"
};
static struct njb_action beam_offsets = {
#include "b_e101_r/files/beam_offsets.inc"
};
static struct njb_action defeat_offsets = {
#include "b_e101_r/files/defeat_offsets.inc"
};
static struct njb_action hover_offsets = {
#include "b_e101_r/files/hover_offsets.inc"
};

/* the trails' objects, whose model and child are byte offsets in E101R.NB */
static NJS_OBJECT trail_0_template = {
#include "b_e101_r/files/trail_0_template.inc"
};
static NJS_OBJECT trail_1_template = {
#include "b_e101_r/files/trail_1_template.inc"
};

Uint8 *e101_r_models = NULL;
NJS_TEXLIST *e101_r_texlist = NULL;
NJS_OBJECT e101_mk2_trail_0_object;
NJS_OBJECT e101_mk2_trail_1_object;
NJS_ACTION e101_mk2_dash_action;
NJS_ACTION e101_mk2_swat1_action;
NJS_ACTION e101_mk2_swat0_action;
NJS_ACTION e101_mk2_dash_end_action;
NJS_ACTION e101_mk2_hurt_action;
NJS_ACTION e101_mk2_hurt_dashing_action;
NJS_ACTION e101_mk2_beam_action;
NJS_ACTION e101_mk2_defeat_action;
NJS_ACTION e101_mk2_hover_action;

static void rebase_action(struct njb_action *offsets, NJS_ACTION *action)
{
    action->object = (NJS_OBJECT *)(e101_r_models + offsets->object);
    action->motion = (NJS_MOTION *)(e101_r_models + offsets->motion);
}

void e101_r_load(void)
{
    Uint8 *memory = (Uint8 *)(((Uint32)e101_r_bss_end + 31) & ~31);

    /* pvb_load loads the textures into texture memory, so E101R.NB can take
     * E101R.PB's place */
    loader_read_file("E101R.PB", memory);
    e101_r_texlist = (NJS_TEXLIST *)pvb_load(memory);
    e101_r_models = memory;
    loader_read_file("E101R.NB", memory);
    njb_relocate(e101_r_models);
    rebase_action(&dash_offsets, &e101_mk2_dash_action);
    rebase_action(&swat1_offsets, &e101_mk2_swat1_action);
    rebase_action(&swat0_offsets, &e101_mk2_swat0_action);
    rebase_action(&dash_end_offsets, &e101_mk2_dash_end_action);
    rebase_action(&hurt_offsets, &e101_mk2_hurt_action);
    rebase_action(&hurt_dashing_offsets, &e101_mk2_hurt_dashing_action);
    rebase_action(&beam_offsets, &e101_mk2_beam_action);
    rebase_action(&defeat_offsets, &e101_mk2_defeat_action);
    rebase_action(&hover_offsets, &e101_mk2_hover_action);
    e101_mk2_trail_0_object = trail_0_template;
    e101_mk2_trail_1_object = trail_1_template;
    /* @hack spelling: the objects and templates reached through their
     * addresses. SHC loads each address from the pool, and reaches model and
     * child at offsets 4 and 44 (0x0c9037a4 to 0x0c90386c); without it, each
     * member's address takes a pool word of its own, 68 of 114 words differ
     * and the function ends 10 bytes late */
    (&e101_mk2_trail_0_object)->model =
        (NJS_MODEL *)(e101_r_models + (Uint32)(&trail_0_template)->model);
    (&e101_mk2_trail_0_object)->child =
        (NJS_OBJECT *)(e101_r_models + (Uint32)(&trail_0_template)->child);
    (&e101_mk2_trail_1_object)->model =
        (NJS_MODEL *)(e101_r_models + (Uint32)(&trail_1_template)->model);
    (&e101_mk2_trail_1_object)->child =
        (NJS_OBJECT *)(e101_r_models + (Uint32)(&trail_1_template)->child);
}

void e101_r_release(void)
{
    pvb_release((struct pvb_texlist *)e101_r_texlist);
    e101_r_texlist = NULL;
}

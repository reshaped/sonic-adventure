/* NJB model and motion banks, and PVB texture banks */
#ifndef SYSTEM_BINARY_H
#define SYSTEM_BINARY_H
#include <ninja.h>
#include <sg_xpt.h>

/* a PVB bank's textures as pvb_load loads them: the texlist, and the two
 * heap arrays under it. the texlist points at names, and each name at its
 * entry in info */
struct pvb_texlist {
    NJS_TEXLIST texlist;
    NJS_TEXINFO *info;
    NJS_TEXNAME *names;
};

/* turn the offsets in the NJB bank at file into pointers, in place, and
 * return TRUE. for a bank of any version but 4, it leaves the bank as it is
 * and returns FALSE. an action's object stays an offset, for the caller to set.
 * file must hold a whole bank: no block's size or offset is checked */
Bool njb_relocate(void *file);
/* an action as two byte offsets from an NJB bank's start */
struct njb_action {
    Uint32 object;
    Uint32 motion;
};
/* load the textures of the PVB bank at file, each under its global
 * index, and return them. it returns NULL when either array's allocation
 * fails. the record's own allocation goes unchecked, and when it fails,
 * pvb_load writes through NULL. file must hold a whole bank, as for
 * njb_relocate */
struct pvb_texlist *pvb_load(void *file);
/* release the textures pvb_load loaded, and free textures and its
 * arrays. textures may be NULL */
void pvb_release(struct pvb_texlist *textures);

#endif

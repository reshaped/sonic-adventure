/* the fish's texture lists, models and actions, which fish.c draws */
#ifndef STG01_FISH_MODELS_H
#define STG01_FISH_MODELS_H

#include <ninja.h>

/* the texlist and model of each kind, in the order fish.c numbers the
 * kinds, 1 to 15. a kind is named for most of its textures, as sb is for
 * f_sb02 to f_sb05 */
extern NJS_TEXLIST fish_sb_texlist;
extern NJS_TEXLIST fish_bb_texlist;
extern NJS_TEXLIST fish_sira_texlist;
extern NJS_TEXLIST fish_tai_texlist;
extern NJS_TEXLIST fish_id_texlist;
extern NJS_TEXLIST fish_kaji_texlist;
extern NJS_TEXLIST fish_sake_texlist;
extern NJS_TEXLIST fish_an_texlist;
extern NJS_TEXLIST fish_pira_texlist;
extern NJS_TEXLIST fish_same_texlist;
extern NJS_TEXLIST fish_hh_texlist;
extern NJS_TEXLIST fish_ms_texlist;
extern NJS_TEXLIST fish_una_texlist;
extern NJS_TEXLIST fish_utu_texlist;
extern NJS_TEXLIST fish_ryu_texlist;
extern NJS_OBJECT fish_sb_object;
extern NJS_OBJECT fish_bb_object;
extern NJS_OBJECT fish_sira_object;
extern NJS_OBJECT fish_tai_object;
extern NJS_OBJECT fish_id_object;
extern NJS_OBJECT fish_kaji_object;
extern NJS_OBJECT fish_sake_object;
extern NJS_OBJECT fish_an_object;
extern NJS_OBJECT fish_pira_object;
extern NJS_OBJECT fish_same_object;
extern NJS_OBJECT fish_hh_object;
extern NJS_OBJECT fish_ms_object;
extern NJS_OBJECT fish_una_object;
extern NJS_OBJECT fish_utu_object;
extern NJS_OBJECT fish_ryu_object;
/* the swim, bite and hooked actions that three families of kinds share.
 * kinds 1 to 7 play sb's, 8 to 12 an's and 13 to 15 una's. each action's
 * object is its family's first model. fish.c plays a copy whose object is
 * the kind's own model */
extern NJS_ACTION fish_sb_swim_action;
extern NJS_ACTION fish_sb_bite_action;
extern NJS_ACTION fish_sb_hooked_action;
extern NJS_ACTION fish_an_swim_action;
extern NJS_ACTION fish_an_bite_action;
extern NJS_ACTION fish_an_hooked_action;
extern NJS_ACTION fish_una_swim_action;
extern NJS_ACTION fish_una_bite_action;
extern NJS_ACTION fish_una_hooked_action;
/* the texlist and actions of kind 8's second model, named for its texture
 * an_ke. fish.c draws the model over an's, posed by its action that matches
 * an's, at the same frame */
extern NJS_TEXLIST fish_an_ke_texlist;
extern NJS_ACTION fish_an_ke_swim_action;
extern NJS_ACTION fish_an_ke_bite_action;
extern NJS_ACTION fish_an_ke_hooked_action;

#endif
